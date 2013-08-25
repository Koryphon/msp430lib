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


#ifndef _FLASH_FS_INTERNAL_H_
#define _FLASH_FS_INTERNAL_H_

#define FFS_UNINIT8            ((uint8_t)FFS_ERASE_VAL)
#define FFS_NULL8            ((uint8_t)~FFS_UNINIT8)
#define FFS_UNINIT16        ((uint16_t)((FFS_UNINIT8<<8)+FFS_UNINIT8))
#define FFS_NULL16            ((uint16_t)((FFS_NULL8<<8)+FFS_NULL8))

//==================================================================================================
// Internal FFS Objects
//==================================================================================================

// Filesystem object
typedef struct
{
    uint16_t BlockSearchStart; // points to the block index where the blocksearch last left off
    uint16_t GetFileCounter; // contains the index of the FT entry to begin the next search at
} FFS_FS_t;

//==================================================================================================
// FLASH Data Structures
//==================================================================================================

// Chunk Header
typedef struct
{
    uint8_t nBytes; // number of bytes in current chunk (including header)
    // addr + nBytes = address of next chunk's header.
    // = 1: Invalid
    // = 2-254: Valid
} FFS_CHDR_t;

//--------------------------------------------------------------------------------------------------
// Block Header - (Also first sector in the block's header)
typedef struct
{
    uint8_t status; // status of the current block
    uint16_t jump; // index of next block. Only valid when status indicates a JUMP

} FFS_SHORT_BHDR_t;

typedef struct
{
    FFS_SHORT_BHDR_t H;
    uint32_t virt_addr; // Virtual address at start of block. Speeds up seeks in large files.
    // EX: for first block, it would be 0. Otherwise, it holds the sum of
    //     file bytes contained in all previous blocks.
} FFS_BHDR_t;

// Block Header statuses
#define FFS_B_UNUSED    FFS_UNINIT8                // Empty block

#define FFS_B_EOF        (FFS_UNINIT8 ^ 0x01)    // Last block in file. Not full
#define FFS_B_JUMP        (FFS_UNINIT8 ^ 0x03)    // Block is full. Use Jump for next block

#define FFS_B_FT_EOF    (FFS_UNINIT8 ^ 0x11)    // Marks block as FT. No more jumps
#define FFS_B_FT_JUMP    (FFS_UNINIT8 ^ 0x13)    // FT Block is definitely full. Jump is valid

#define FFS_TST_EOF(x)        ((x & 0x0F) == ((FFS_UNINIT8 & 0x0F)^0x01))
#define FFS_TST_JUMP(x)        ((x & 0x0F) == ((FFS_UNINIT8 & 0x0F)^0x03))
#define FFS_TST_FT(x)        ((x & 0xF0) == ((FFS_UNINIT8 & 0xF0)^0x10))

//--------------------------------------------------------------------------------------------------
// File table entry (FTE)
typedef struct
{
    uint16_t startblock; // points to start block. if == FFS_NULL16: entry is marked for deletion
    char filename[FFS_FILENAME_LEN];
} FFS_FTE_t;
#define FFS_FTENTRIESPERBLOCK    ((FLASH_BLOCKSIZE-sizeof(FFS_SHORT_BHDR_t))/sizeof(FFS_FTE_t))
#define FFS_ENTRYADDR(block,entry)    (block*FLASH_BLOCKSIZE+sizeof(FFS_SHORT_BHDR_t)+entry*sizeof(FFS_FTE_t))

//--------------------------------------------------------------------------------------------------
// File Entry Info Object (Not used in actual flash. Used as a wrapper when accessing an FTE)
typedef struct
{
    FFS_FTE_t FTE; // actual file table entry
    uint32_t fteAddr; // hw address of file table entry
} FFS_FTE_INFO_t;


#endif
