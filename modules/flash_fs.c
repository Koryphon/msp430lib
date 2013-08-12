/*
* Copyright (c) 2012, Alexander I. Mykyta
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*==================================================================================================
* File History:
* NAME          DATE         COMMENTS
* Alex M.       2011-07-02   born
* Alex M.       2012-04-17   Fixed block uint32_t multiplication bug
*
*=================================================================================================*/

/**
* \addtogroup MOD_FLASHFS
* \{
**/

/**
* \file
* \brief Code for \ref MOD_FLASHFS "Flash File System"
* \author Alex Mykyta
**/

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <result.h>
#include "FlashSPAN.h"
#include "flash_fs.h"
#include "flash_fs_internal.h"

static FFS_FS_t FS;

//==================================================================================================
// Internal Functions
//==================================================================================================
static uint16_t FindUnusedBlock(void)
{
    // returns the block ID of a free block. If no free blocks found, Returns 0
    uint8_t status;
    uint16_t block;
    uint16_t startingblock;

    block = FS.BlockSearchStart;
    startingblock = block;

    do {
        block++;

        if (block == flashSPAN.BlockCount) {
            block = 1; // wrap to beginning. Block 0 is always used so skip it.
            if (startingblock == 0) {
                return(0); // wrapped on initial block search
            }
        }

        // read the next block header
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE + offsetof(FFS_BHDR_t, H.status), &status, sizeof(status));

        if (block == startingblock) {
            // Has scanned all the blocks
            if (status == FFS_B_UNUSED) {
                return(block);
            }
            else {
                return(0);
            }
        }

    }
    while (status != FFS_B_UNUSED);

    FS.BlockSearchStart = block;
    return(block);
}
//--------------------------------------------------------------------------------------------------
static void WR_SeekToEOF(FFS_FILE_t* FILE)
{
    // given a file object, follow the jump-chain until the block is marked EOF
    // Seek to the next writeable position in the block
    // Updates the file object to point to the next unused byte
    // ONLY GOOD FOR A FLUSHED WRITE MODE FILE

    FFS_BHDR_t BHDR;
    FFS_CHDR_t CHDR;
    uint32_t addr;
    uint16_t block;
    uint32_t virt_addr;
    uint32_t block_addr;

    block = FILE->startblock;
    flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));

    while (BHDR.H.status == FFS_B_JUMP) { // loop while there exists a next block
        block = BHDR.H.jump;
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
    }

    virt_addr = BHDR.virt_addr;
    block_addr = ((uint32_t)block) * FLASH_BLOCKSIZE;
    addr = block_addr + sizeof(BHDR);

    while (addr - block_addr <= FLASH_BLOCKSIZE - (sizeof(CHDR) + 1)) {
        // while inside the current block
        flashSPAN_Read(addr, (uint8_t*)&CHDR, sizeof(CHDR));
        if (CHDR.nBytes == FFS_UNINIT8) {
            // reached a writeable location in the block
            FILE->hw_addr = addr;
            FILE->virt_addr = virt_addr;
            FILE->nBytes = 1;
            return;
        }
        // otherwise, increment appropriately
        addr += CHDR.nBytes;
        virt_addr += CHDR.nBytes - sizeof(CHDR);
    }

    // If program reached here, EOF Block is full.
    FILE->hw_addr = block_addr;
    FILE->virt_addr = virt_addr;
    FILE->nBytes = 0;

}
//--------------------------------------------------------------------------------------------------
static RES_t LookupFile(FFS_FTE_INFO_t *FTEI, char *filename)
{
    // looks up the file in the file table based on the filename
    // If the file is found, FTEI is filled with its entry info
    // If not found, FTEI holds the last address checked
    //    If FTEI contains an entry, then it is the last entry in the block and it is full.
    FFS_SHORT_BHDR_t SBHDR;
    uint16_t entry;
    uint16_t block;


    block = 0; // FT always starts at block 0
    do {
        // check each entry in the FT Block
        for (entry = 0; entry < FFS_FTENTRIESPERBLOCK; entry++) {
            FTEI->fteAddr = FFS_ENTRYADDR(block, entry);
            flashSPAN_Read(FTEI->fteAddr, (uint8_t*)&FTEI->FTE, sizeof(FFS_FTE_t)); // read FTE
            if (FTEI->FTE.startblock == FFS_NULL16) {
                // entry marked for deletion. Skip it
            }
            else if (FTEI->FTE.startblock == FFS_UNINIT16) {
                // entry is unused
                // reached end of FT entries.
                return(RES_NOTFOUND);
            }
            else {
                // Valid File
                if (strcmp(filename, FTEI->FTE.filename) == 0) {
                    // found a match
                    return(RES_OK);
                }
            }
        }

        // Read the current FT block's header
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR));
        block = SBHDR.jump; // Only valid if status == JUMP.
    }
    while (SBHDR.status == FFS_B_FT_JUMP); // loop if status == JUMP

    return(RES_NOTFOUND);
}

//==================================================================================================
// Filesystem operations
//==================================================================================================

RES_t ffs_init(void)
{
    // Initializes the filesystem
    // check first block to see if a filetable block exists.
    // If not, erase all devices and write a FT header
    uint8_t status;

    if (flashSPAN_Init() != RES_OK) {
        return(RES_FAIL);
    }

    // check for FT
    flashSPAN_Read(offsetof(FFS_BHDR_t, H.status), &status, sizeof(status));
    if (!FFS_TST_FT(status)) { // if it is not marked as an FT
        // Bad FT, erase all and make one.
        flashSPAN_EraseAll();
        status = FFS_B_FT_EOF;
        flashSPAN_Write(offsetof(FFS_BHDR_t, H.status), &status, sizeof(status));
    }

    //pre-search for the next unused block.
    FS.BlockSearchStart = 0;
    FS.BlockSearchStart = FindUnusedBlock() - 1;

    FS.GetFileCounter = 0;

    return(RES_OK);
}
//--------------------------------------------------------------------------------------------------
RES_t ffs_cleanupFT(void)
{
    // de-fragments the filetable.
#if (FFS_CLEANUP_FT_MODE == 0)
    // Local Buffer
    FFS_FTE_t FTEBuf[FFS_FTENTRIESPERBLOCK];
    uint16_t oldentry, newentry;
    uint16_t oldblock, newblock, prevnewblock;
    FFS_SHORT_BHDR_t SBHDR;
    FFS_SHORT_BHDR_t newSBHDR;

    FS.GetFileCounter = 0;

    // Search copy FTEs into buffer until it is full
    oldentry = 0;
    oldblock = 0;
    prevnewblock = 0;
    newblock = 0;
    flashSPAN_Read(0, (uint8_t*)&SBHDR, sizeof(SBHDR));

    while (1) {
        // loop for each new block
        newentry = 0;

        while (newentry < FFS_FTENTRIESPERBLOCK) {
            //seek through old entries until enough are gathered for a new block
            flashSPAN_Read(FFS_ENTRYADDR(oldblock, oldentry), (uint8_t*)&FTEBuf[newentry], sizeof(FFS_FTE_t));
            if (FTEBuf[newentry].startblock == FFS_NULL16) {
                // FTE is marked for delete. Skip
                oldentry++;
            }
            else if (FTEBuf[newentry].startblock == FFS_UNINIT16) {
                // Reached end of old FTEs
                // erase the current block
                flashSPAN_EraseBlock(oldblock);
                oldentry = FFS_FTENTRIESPERBLOCK; // mark as done
                break;
            }
            else {
                // Valid FTE.
                oldentry++;
                newentry++;
            }


            if (oldentry == FFS_FTENTRIESPERBLOCK) {
                // Reached end of old block.
                // Erase it
                flashSPAN_EraseBlock(oldblock);
                if (SBHDR.status == FFS_B_FT_JUMP) {
                    //jump to next
                    oldblock = SBHDR.jump;
                    flashSPAN_Read(((uint32_t)oldblock) * FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(FFS_SHORT_BHDR_t));
                    oldentry = 0;
                }
                else {
                    // No more jumps, reached end of old FTEs
                    oldentry = FFS_FTENTRIESPERBLOCK; // mark as done
                    break;
                }
            }
        }

        // Write copied entries to a new block
        if ((newblock == 0) && (prevnewblock == 0)) { // detect initial condition
            // first newblock
            // reserve the current new block
            newSBHDR.status = FFS_B_FT_EOF;
            flashSPAN_Write(offsetof(FFS_SHORT_BHDR_t, status), &newSBHDR.status, sizeof(newSBHDR.status));

            // write FTEs
            flashSPAN_Write(sizeof(FFS_SHORT_BHDR_t), (uint8_t*)FTEBuf, newentry * sizeof(FFS_FTE_t));
            prevnewblock = 1; // unset initial condition
        }
        else {
            prevnewblock = newblock;
            newblock = FindUnusedBlock();
            if (newblock == 0) {
                // SHOULD NEVER HAPPEN. FATAL ERROR
                newblock = 0;
            }

            // reserve the current new block
            newSBHDR.status = FFS_B_FT_EOF;
            flashSPAN_Write(((uint32_t)newblock)*FLASH_BLOCKSIZE + offsetof(FFS_SHORT_BHDR_t, status),
                            &newSBHDR.status, sizeof(newSBHDR.status));

            // Write the FTEs
            flashSPAN_Write(((uint32_t)newblock) * FLASH_BLOCKSIZE + sizeof(FFS_SHORT_BHDR_t), (uint8_t*)FTEBuf, newentry * sizeof(FFS_FTE_t));

            // write prev header
            newSBHDR.jump = newblock;
            newSBHDR.status = FFS_B_FT_JUMP;
            flashSPAN_Write(((uint32_t)prevnewblock) * FLASH_BLOCKSIZE, (uint8_t*)&newSBHDR, sizeof(FFS_SHORT_BHDR_t));
        }

        // if hit the end of the old FTEs
        if (oldentry == FFS_FTENTRIESPERBLOCK) {
            return(RES_OK);
        }
    }


#else
    ///\todo Flash Scratchpad method of filetable garbage collection

#endif
}

//--------------------------------------------------------------------------------------------------
uint16_t ffs_countGarbageFTE(void)
{
    FFS_SHORT_BHDR_t SBHDR;
    uint16_t entry;
    uint16_t block;
    uint16_t startblock;
    uint16_t garbagecount = 0;

    block = 0; // FT always starts at block 0
    do {
        // check each entry in the FT Block
        for (entry = 0; entry < FFS_FTENTRIESPERBLOCK; entry++) {

            // read FTE's startblock
            flashSPAN_Read(FFS_ENTRYADDR(block, entry), (uint8_t*)&startblock, sizeof(startblock));
            if (startblock == FFS_NULL16) {
                // entry marked for deletion.
                garbagecount++;
            }
            else if (startblock == FFS_UNINIT16) {
                // entry is unused
                // reached end of FT entries.
                return(garbagecount);
            }
            else {
                // Valid File. Skip it.
            }
        }

        // Read the current FT block's header
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR));
        block = SBHDR.jump; // Only valid if status == JUMP.
    }
    while (SBHDR.status == FFS_B_FT_JUMP); // loop if status == JUMP

    return(garbagecount);
}

//--------------------------------------------------------------------------------------------------
uint16_t ffs_blocksFree(void)
{
    // returns the number of free blocks
    uint16_t freecount;
    uint16_t firstblock;
    freecount = 1;

    firstblock = FindUnusedBlock();
    if (firstblock == 0) {
        return(0);
    }

    while (FindUnusedBlock() != firstblock) {
        freecount++;
    }
    FS.BlockSearchStart = firstblock - 1;
    return(freecount);
}
//==================================================================================================
// File Operations
//==================================================================================================
RES_t ffs_fopen(FFS_FILE_t* FILE, char *filename, FFS_FILEMODE_t filemode)
{
    // Opens a file based on the filemode given a filename string
    // if FILE's readmode is not CLOSED, flag an error
    FFS_FTE_INFO_t FTEI;
    RES_t result;
    uint16_t block;
    uint16_t fteBlock;

    FFS_BHDR_t BHDR;
    FFS_CHDR_t CHDR;

    if (FILE == NULL) {
        return(RES_PARAMERR);
    }

    result = LookupFile(&FTEI, filename);

    switch (filemode) {
    case FFS_RD:
        if (result == RES_OK) {
            // file found. Populate FILE object pointing to beginning of file

            FILE->filemode = FFS_RD;
            FILE->startblock = FTEI.FTE.startblock;
            FILE->virt_addr = 0;
            FILE->hw_addr = ((uint32_t)FTEI.FTE.startblock) * FLASH_BLOCKSIZE + sizeof(BHDR);
            flashSPAN_Read(FILE->hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
            if (CHDR.nBytes == FFS_UNINIT8) {
                // empty file. mark as EOF
                FILE->nBytes = 0;
            }
            else {
                FILE->nBytes = CHDR.nBytes - 1;
                FILE->hw_addr += 1;
            }
        }
        else {
            return(result); // pass on any error.
        }
        break;
    case FFS_WR_REPLACE: // if replace or append...
    case FFS_WR_APPEND:
        if (result == RES_NOTFOUND) {
            // not found. Create a new file
            // FTEI holds the last entry searched. If it is empty, then write an entry there.
            // otherwise, find a new block for the FT and make a new entry there.

            fteBlock = FTEI.fteAddr / FLASH_BLOCKSIZE;

            if (FTEI.FTE.startblock != FFS_UNINIT16) {
                // the FT must be expanded to another block
                block = FindUnusedBlock();
                if (block == 0) {
                    // could not find a new block
                    return(RES_FULL);
                }

                // write jump info to current block
                BHDR.H.status = FFS_B_FT_JUMP;
                BHDR.H.jump = block;
                flashSPAN_Write(((uint32_t)fteBlock)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR.H, sizeof(BHDR.H));

                // write EOF header to new block
                BHDR.H.status = FFS_B_FT_EOF;
                flashSPAN_Write(((uint32_t)block)*FLASH_BLOCKSIZE + offsetof(FFS_BHDR_t, H.status), &BHDR.H.status, sizeof(BHDR.H.status));

                FTEI.fteAddr = ((uint32_t)block) * FLASH_BLOCKSIZE + sizeof(BHDR.H);
                fteBlock = block;
            }

            // look for next empty block.
            block = FindUnusedBlock();
            if (block == 0) {
                // could not find a new block
                return(RES_FULL);
            }

            //  Initialize first block's BHDR
            BHDR.H.status = FFS_B_EOF;
            BHDR.H.jump = FFS_UNINIT16;
            BHDR.virt_addr = 0;
            flashSPAN_Write(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));

            // Write file entry
            FTEI.FTE.startblock = block;
            strcpy(FTEI.FTE.filename, filename);
            flashSPAN_Write(FTEI.fteAddr, (uint8_t*)&FTEI.FTE, sizeof(FTEI.FTE));

            // Populate FILE object
            FILE->startblock = block;
            FILE->filemode = FFS_WR_APPEND;
            FILE->hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE + sizeof(BHDR);
            FILE->virt_addr = 0;
            FILE->nBytes = 1;
        }
        else if (result == RES_OK) {
            // File exists
            if (filemode == FFS_WR_APPEND) {
                // Append mode. Seek to end
                FILE->filemode = FFS_WR_APPEND;
                FILE->startblock = FTEI.FTE.startblock;
                WR_SeekToEOF(FILE);
            }
            else {
                // Replace mode. Delete data block chain

                block = FTEI.FTE.startblock; // first block in chain
                // erase all the file's blocks
                do {
                    flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
                    flashSPAN_EraseBlock(block);
                    block = BHDR.H.jump;
                }
                while (BHDR.H.status == FFS_B_JUMP);

                // re-initialize first block
                BHDR.H.status = FFS_B_EOF;
                BHDR.H.jump = FFS_UNINIT16;
                BHDR.virt_addr = 0;
                flashSPAN_Write(((uint32_t)FTEI.FTE.startblock)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));

                // setup FILE object
                FILE->startblock = FTEI.FTE.startblock;
                FILE->filemode = FFS_WR_APPEND; // Behaves the same as APPEND after being replaced.
                FILE->hw_addr = ((uint32_t)FTEI.FTE.startblock) * FLASH_BLOCKSIZE + sizeof(BHDR);
                FILE->virt_addr = 0;
                FILE->nBytes = 1;
            }
        }
        else {
            return(result); // pass on any error.
        }
        break;
    default:
        return(RES_PARAMERR);
    }
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
RES_t ffs_fclose(FFS_FILE_t* FILE)
{
    // Closes the FILE object

    if (FILE == NULL) {
        return(RES_PARAMERR);
    }

    switch (FILE->filemode) {
    case FFS_RD: //RD - simply change FILE's filemode to CLOSED
        FILE->filemode = FFS_CLOSED;
        break;
    case FFS_WR_APPEND: //WR - perform a flush and then change filemode to closed
        if (FILE->nBytes > sizeof(FFS_CHDR_t)) {
            ffs_fflush(FILE);
        }
        FILE->filemode = FFS_CLOSED;
        break;
    default:
        return(RES_PARAMERR);
    }
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
size_t ffs_fwrite( uint8_t *data, size_t size, FFS_FILE_t* FILE )
{
    // Writes to the file specified by FILE.
    FFS_BHDR_t BHDR;
    FFS_CHDR_t CHDR;
    uint16_t block;
    uint16_t writelen;
    uint16_t block_remaining; // bytes remaining in block
    size_t size_done = 0; // number of bytes successfully accessed

    // Temporary FILE variables:
    uint32_t virt_addr;
    uint32_t hw_addr;
    uint8_t nBytes;

    if (FILE->filemode != FFS_WR_APPEND) {
        return(0);
    }

    virt_addr = FILE->virt_addr;
    hw_addr = FILE->hw_addr;
    nBytes = FILE->nBytes;

    if (nBytes == 0) {
        //Current block is full. Find new block
        // (hw_addr holds start addr of current block)
        block = FindUnusedBlock();
        if (block == 0) {
            return(0);
        }

        // initialize new block
        BHDR.H.status = FFS_B_EOF;
        BHDR.H.jump = FFS_UNINIT16;
        BHDR.virt_addr = virt_addr;
        flashSPAN_Write(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));

        // Update old block's header
        BHDR.H.status = FFS_B_JUMP;
        BHDR.H.jump = block;
        flashSPAN_Write(hw_addr, (uint8_t*)&BHDR, sizeof(BHDR) - sizeof(BHDR.virt_addr));

        hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE + sizeof(BHDR);
        nBytes = sizeof(CHDR);
    }

    // At this point, nBytes is definitely not 0 and hw_addr points to a valid location
    block = hw_addr / FLASH_BLOCKSIZE; // current block
    block_remaining = FLASH_BLOCKSIZE - ((hw_addr + nBytes) - ((uint32_t)block) * FLASH_BLOCKSIZE); // - sizeof(CHDR);
    while (size > size_done) {

        // what is the maximum I can write into this chunk?
        if (size - size_done > (254 - nBytes)) {
            writelen = (254 - nBytes);
        }
        else {
            writelen = size - size_done;
        }
        if (writelen > block_remaining) {
            writelen = block_remaining;
        }

        flashSPAN_Write(hw_addr + nBytes, data, writelen);
        nBytes += writelen;
        virt_addr += writelen;
        size_done += writelen;
        data += writelen;
        block_remaining -= writelen;

        if ((nBytes == 254) && (block_remaining > sizeof(CHDR))) {
            // reached the end of the chunk but not the end of the block

            // Close this one and find the next one
            flashSPAN_Write(hw_addr, &nBytes, sizeof(nBytes));
            hw_addr += 254;
            nBytes = sizeof(CHDR);
            block_remaining -= sizeof(CHDR);
        }
        else if (block_remaining <= sizeof(CHDR)) {
            // reached the end of the block

            // close the chunk
            flashSPAN_Write(hw_addr, &nBytes, sizeof(nBytes));


            if (size == size_done) {
                // Finished writing. Block is full
                FILE->virt_addr = virt_addr;
                FILE->hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE;
                FILE->nBytes = 0;
                return(size_done);
            }
            // Still need to write. Find a new block
            hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE;
            block = FindUnusedBlock();
            if (block == 0) {
                FILE->virt_addr = virt_addr;
                FILE->hw_addr = hw_addr;
                FILE->nBytes = 0;
                return(size_done);
            }

            // reserve new block
            BHDR.H.status = FFS_B_EOF;
            BHDR.H.jump = FFS_UNINIT16;
            BHDR.virt_addr = virt_addr;
            flashSPAN_Write(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));

            // Update old block's header
            BHDR.H.status = FFS_B_JUMP;
            BHDR.H.jump = block;
            flashSPAN_Write(hw_addr, (uint8_t*)&BHDR, sizeof(BHDR) - sizeof(BHDR.virt_addr));

            hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE + sizeof(BHDR);
            nBytes = sizeof(CHDR);
            block_remaining = FLASH_BLOCKSIZE - sizeof(BHDR) - sizeof(CHDR);//*2;
        }
    }

    FILE->hw_addr = hw_addr;
    FILE->nBytes = nBytes;
    FILE->virt_addr = virt_addr;

    return(size_done);
}

//--------------------------------------------------------------------------------------------------
size_t ffs_fread( uint8_t *data, size_t size, FFS_FILE_t* FILE )
{
    // Reads sequentially from a file
    FFS_BHDR_t BHDR;
    FFS_CHDR_t CHDR;
    uint16_t readlen;
    uint16_t block;
    size_t size_done = 0; // number of bytes successfully accessed

    // Temporary FILE variables:
    uint32_t virt_addr;
    uint32_t hw_addr;
    uint8_t nBytes;

    if (FILE->filemode != FFS_RD) {
        return(0);
    }

    virt_addr = FILE->virt_addr;
    hw_addr = FILE->hw_addr;
    nBytes = FILE->nBytes;

    if (nBytes == 0) {
        // Previous read hit the EOF. Is it still the EOF?
        if ((hw_addr % FLASH_BLOCKSIZE) == 0) {
            // block was full. Has a jump been generated?
            flashSPAN_Read(hw_addr, (uint8_t*)&BHDR, sizeof(BHDR));
            if (BHDR.H.status != FFS_B_JUMP) {
                return(0);
            }
            // follow jump and find next readable address
            hw_addr = ((uint32_t)BHDR.H.jump) * FLASH_BLOCKSIZE + sizeof(BHDR);
            flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));

            if (CHDR.nBytes == FFS_UNINIT8) {
                // I just jumped to an empty block! wtf
                // update file object so the work done is not wasted.
                FILE->hw_addr = hw_addr;
                // FILE->nBytes is already 0.
                return(0);
            }
            // found a new chunk!
            hw_addr += sizeof(CHDR);
            nBytes = CHDR.nBytes - sizeof(CHDR);

        }
        else {
            // Is there a next chunk?
            flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
            if (CHDR.nBytes == FFS_UNINIT8) {
                // Nope. Still EOF
                return(0);
            }
            // Yes there is. Init stuff to point to it.
            hw_addr += sizeof(CHDR);
            nBytes = CHDR.nBytes - sizeof(CHDR);
        }
    }


    // At this point, nBytes is definitely not 0 and hw_addr points to a valid location
    block = hw_addr / FLASH_BLOCKSIZE; // current block

    while (size > size_done) {

        // what is the maximum I can read from this chunk?
        if (size - size_done > nBytes) {
            readlen = nBytes;
        }
        else {
            readlen = size - size_done;
        }

        flashSPAN_Read(hw_addr, data, readlen);
        nBytes -= readlen;
        hw_addr += readlen;
        virt_addr += readlen;
        size_done += readlen;
        data += readlen;


        if (nBytes == 0) {
            // reached the end of the chunk. Find the next one
            if (hw_addr - ((uint32_t)block * FLASH_BLOCKSIZE) <= FLASH_BLOCKSIZE - (sizeof(CHDR) + 1)) {
                // Still in the block.
                flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
                if (CHDR.nBytes == FFS_UNINIT8) {
                    // reached EOF
                    FILE->hw_addr = hw_addr;
                    FILE->nBytes = 0;
                    FILE->virt_addr = virt_addr;
                    return(size_done);
                }
                else {
                    // setup next chunk
                    hw_addr += sizeof(CHDR);
                    nBytes = CHDR.nBytes - sizeof(CHDR);
                }
            }
            else {
                // Reached end of block
                flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
                if (BHDR.H.status == FFS_B_JUMP) {
                    // follow jump and find next readable address
                    hw_addr = ((uint32_t)BHDR.H.jump) * FLASH_BLOCKSIZE + sizeof(BHDR);
                    flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
                    if (CHDR.nBytes != FFS_UNINIT8) {
                        // found a new chunk!
                        block = BHDR.H.jump;
                        hw_addr += sizeof(CHDR);
                        nBytes = CHDR.nBytes - sizeof(CHDR);
                    }
                    else {
                        // I just jumped to an empty block! wtf!
                        FILE->hw_addr = hw_addr;
                        FILE->nBytes = 0;
                        FILE->virt_addr = virt_addr;
                        return(size_done);
                    }
                }
                else {
                    // block is full.
                    FILE->hw_addr = ((uint32_t)block) * FLASH_BLOCKSIZE;
                    FILE->nBytes = 0;
                    FILE->virt_addr = virt_addr;
                    return(size_done);
                }
            }
        }
    }
    FILE->hw_addr = hw_addr;
    FILE->nBytes = nBytes;
    FILE->virt_addr = virt_addr;

    return(size_done);
}

//--------------------------------------------------------------------------------------------------
RES_t ffs_fseek(FFS_FILE_t* FILE, uint32_t offset)
{
    // Seeks the FILE location to the byte offset.
    // HOW: Follow jump chain appropriately. If offset ends up exceeding the file length, return EOF
    uint16_t block, prevblock;
    FFS_BHDR_t BHDR;
    uint32_t hw_addr, virt_addr;
    FFS_CHDR_t CHDR;


    if (FILE->filemode != FFS_RD) {
        return(RES_PARAMERR);
    }

    block = FILE->hw_addr / FLASH_BLOCKSIZE;

    // check if it is before the current block
    flashSPAN_Read((uint32_t)block * FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
    if (BHDR.virt_addr > offset) {
        // rewind to beginning
        block = FILE->startblock;
        flashSPAN_Read((uint32_t)block * FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
    }

    // seek to the containing block
    while (1) {
        if (BHDR.H.status != FFS_B_JUMP) {
            // reached EOF block
            virt_addr = BHDR.virt_addr;
            break;
        }
        else {
            // follow jump
            prevblock = block;
            virt_addr = BHDR.virt_addr;
            block = BHDR.H.jump;
            flashSPAN_Read((uint32_t)block * FLASH_BLOCKSIZE, (uint8_t*)&BHDR, sizeof(BHDR));
            if (BHDR.virt_addr > offset) {
                // overshot the offset
                block = prevblock;
                break;
            }
        }
    }

    // seek within block to the address
    hw_addr = ((uint32_t)block * FLASH_BLOCKSIZE) + sizeof(BHDR);
    flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
    while (1) {
        if (virt_addr + (CHDR.nBytes - sizeof(CHDR)) <= offset) {
            // next chunk
            hw_addr += CHDR.nBytes;
            virt_addr += CHDR.nBytes - sizeof(CHDR);
            if (hw_addr - ((uint32_t)block * FLASH_BLOCKSIZE) > FLASH_BLOCKSIZE - (sizeof(CHDR) + 1)) {
                // reached end of block. EOF
                FILE->hw_addr = (uint32_t)block * FLASH_BLOCKSIZE;
                FILE->nBytes = 0;
                FILE->virt_addr = virt_addr;
                return(RES_END);
            }
            flashSPAN_Read(hw_addr, (uint8_t*)&CHDR, sizeof(CHDR));
            if (CHDR.nBytes == FFS_UNINIT8) {
                // Empty chunk. EOF
                FILE->hw_addr = hw_addr;
                FILE->nBytes = 0;
                FILE->virt_addr = virt_addr;
                return(RES_END);
            }
        }
        else {
            // end is in this chunk
            virt_addr = offset - virt_addr;
            FILE->nBytes = CHDR.nBytes - virt_addr - sizeof(CHDR);
            FILE->hw_addr = hw_addr + virt_addr + sizeof(CHDR);
            FILE->virt_addr = offset;
            return(RES_OK);
        }
    }
}

//--------------------------------------------------------------------------------------------------
uint32_t  ffs_ftell(FFS_FILE_t* FILE)
{
    return(FILE->virt_addr);
}
//--------------------------------------------------------------------------------------------------
int ffs_feof(FFS_FILE_t* FILE)
{
    // returns boolean of whether or not read pointer is at EOF
    if (FILE->filemode == FFS_RD) {
        return(!(FILE->nBytes));
    }
    else {
        return(0);
    }
}
//--------------------------------------------------------------------------------------------------
RES_t ffs_remove(char *filename)
{
    FFS_FTE_INFO_t FTEI;
    FFS_SHORT_BHDR_t SBHDR;
    uint16_t block;

    if (LookupFile(&FTEI, filename) == RES_OK) {
        block = FTEI.FTE.startblock; // first block in chain

        // Mark FTE as deleted
        FTEI.FTE.startblock = FFS_NULL16;
        flashSPAN_Write(FTEI.fteAddr + offsetof(FFS_FTE_t, startblock),
                        (uint8_t*)&FTEI.FTE.startblock, sizeof(FTEI.FTE.startblock));

        // Erase file's blocks
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR)); // read first SBHDR
        flashSPAN_EraseBlock(block);    // delete first block

        // erase the rest of the file's blocks
        while (SBHDR.status == FFS_B_JUMP) { // while there is another block to be jumped to:
            block = SBHDR.jump;
            flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR));
            flashSPAN_EraseBlock(block);
        }
    }
    return(RES_OK);
}
//--------------------------------------------------------------------------------------------------
RES_t ffs_fflush(FFS_FILE_t* FILE)
{
    uint32_t chunk_addr;
    // Temporary FILE variables:
    uint32_t hw_addr;
    uint8_t nBytes;

    if (FILE->filemode != FFS_WR_APPEND) {
        return(RES_PARAMERR);
    }

    if (FILE->nBytes > sizeof(FFS_CHDR_t)) {
        // data has been written to chunk
        hw_addr = FILE->hw_addr;
        nBytes = FILE->nBytes;

        chunk_addr = hw_addr - (hw_addr % FLASH_BLOCKSIZE);
        flashSPAN_Write(hw_addr, &nBytes, sizeof(nBytes));
        hw_addr += nBytes;

        if (hw_addr - chunk_addr > FLASH_BLOCKSIZE - sizeof(FFS_CHDR_t) - 1) {
            // Block is full
            hw_addr = chunk_addr;
            nBytes = 0;
        }
        else {
            nBytes = sizeof(FFS_CHDR_t);
        }
        FILE->hw_addr = hw_addr;
        FILE->nBytes = nBytes;
    }
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
RES_t ffs_getFile(char *filename)
{
    // Gets the filename of the next file in the filetable.
    // string is returned to buffer pointed to by *filename
    // passing in a null pointer resets the filetable counter
    FFS_FTE_t FTE;
    FFS_SHORT_BHDR_t SBHDR;
    uint16_t block;
    uint16_t counter;
    uint16_t entry, startentry;
    uint32_t fteAddr;

    if (filename == NULL) {
        FS.GetFileCounter = 0;
        return(RES_OK);
    }
    block = 0;
    counter = 0;

    // jump up to correct FT block
    while (FS.GetFileCounter >= counter + FFS_FTENTRIESPERBLOCK) {
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR));
        if (SBHDR.status != FFS_B_FT_JUMP) {
            // cannot jump any further. Counter value is invalid
            FS.GetFileCounter = 0;
            return(RES_END);
        }
        block = SBHDR.jump;
        counter += FFS_FTENTRIESPERBLOCK;
    }

    startentry = FS.GetFileCounter % FFS_FTENTRIESPERBLOCK;
    do {
        // check each entry in the FT Block
        for (entry = startentry; entry < FFS_FTENTRIESPERBLOCK; entry++) {
            fteAddr = FFS_ENTRYADDR(block, entry);
            flashSPAN_Read(fteAddr, (uint8_t*)&FTE, sizeof(FTE)); // read FTE
            if (FTE.startblock == FFS_NULL16) {
                // entry marked for deletion. Skip it
            }
            else if (FTE.startblock == FFS_UNINIT16) {
                // entry is unused
                // reached end of FT entries.
                FS.GetFileCounter = 0;
                return(RES_END);
            }
            else {
                // Valid File
                strcpy(filename, FTE.filename);
                FS.GetFileCounter = counter + entry + 1;
                return(RES_OK);
            }
        }

        // Read the current FT block's header
        flashSPAN_Read(((uint32_t)block)*FLASH_BLOCKSIZE, (uint8_t*)&SBHDR, sizeof(SBHDR));
        block = SBHDR.jump; // Only valid if status == JUMP.
        counter += FFS_FTENTRIESPERBLOCK;
        startentry = 0;
    }
    while (SBHDR.status == FFS_B_FT_JUMP); // loop if status == JUMP

    // reached the end of the last block
    FS.GetFileCounter = 0;
    return(RES_OK);
}

///\}
