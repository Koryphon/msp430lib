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

/**
* \addtogroup MOD_FLASHFS Flash File System
* \brief Light-weight file system for Flash volumes
* \author Alex Mykyta
*
* Flash File System (FFS) is a simple file system designed for Flash volumes. Since Flash devices
* can typically only be erased in large blocks, FFS is designed to accommodate for
* this restriction. \n
* In the interest of keeping FFS light-weight, directories are not supported.\n
*
* \ref MOD_FLASHFS Flash "File System" also requires the following modules:
*    - \ref MOD_FLASHSPAN "Spanned Flash Memory Volume" (Requires other modules depending on implementation)
*
* <b> Compilers Supported: </b>
*    - Any C89 compatible or newer
*
* \todo If a file is not closed cleanly (power failure) implement a way so that the next time it is
* opened, the end of the file can be cleaned up. No guarantees of total preservation but better than
* nothing. Possibly scan the remainder of a possible chunk and truncate the unclosed chunk assuming
* that 0xFF means the end.
*
* \todo Make chunk headers 2-bytes instead of 1-byte. This will improve filesystem efficiency
*
*
* \{
**/

/**
* \file
* \brief Include file for \ref MOD_FLASHFS "Flash File System"
* \author Alex Mykyta
**/

#ifndef _FLASH_FS_H_
#define _FLASH_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "flash_fs_config.h"
#include <result.h>

//==================================================================================================
// Enumerations
//==================================================================================================
    enum FFS_FILEMODE {
        FFS_CLOSED = 0,        ///< File is not open (Used internally)
        FFS_RD,                ///< Open file for reading
        FFS_WR_APPEND,        ///< Open file for writing. Appends data. Creates new file if needed.
        FFS_WR_REPLACE        ///< Open file for writing. Replaces file. Creates new file if needed.
    };
    typedef uint8_t FFS_FILEMODE_t;

//==================================================================================================
// FFS Objects
//==================================================================================================

// File object
    typedef struct {
        uint32_t virt_addr; // current virtual address (the offset within a file)
        uint32_t hw_addr;
        // WRITE: Points to the hardware start address of the current chunk
        // READ: Points to the next hardware address to be read
        uint8_t nBytes;
        // WRITE: # of bytes in current chunk (Incl Header)
        //  = 0:        Current block is full. Find new block on next write.
        //                hw_addr points to the start of the full block
        //  = 1:        Chunk is valid. No data has been written. Next write address = hw_addr+nBytes
        //  = 2-254:    Data has been written to the current chunk. Next write address = hw_addr+nBytes
        //  = 255:    Invalid
        // READ: # of bytes remaining in current chunk.
        //  = 0:        Nothing left to read. Reached EOF. hw_addr points to next chunk header.
        //                If block is full, hw_addr points to the start of the block
        //  = 1-253:    Next read address = hw_addr. Next chunk header = hw_addr + nBytes (if in block)
        FFS_FILEMODE_t filemode; // access mode of the file
        uint16_t startblock; // start block of the file
    } FFS_FILE_t;

//==================================================================================================
// Function Prototypes
//==================================================================================================

///\name Filesystem operations
///\{

    /**
    * \brief Initializes the filesystem
    * \retval    RES_FAIL    Initialization of the storage medium failed
    * \retval    RES_OK        Success!
    *
    * If the storage medium does not contain a properly formatted filesystem, it will be erased and a
    * new one created.
    **/
    RES_t ffs_init(void);

    /**
    * \brief Scans the file table for garbage entries.
    * \returns    Number of garbage file table entries
    *
    * If there is a large amount of garbage file table entries, it is recommended to run ffs_cleanupFT()
    **/
    uint16_t ffs_countGarbageFTE(void);

    /**
    * \brief De-fragments the file table.
    * \retval    RES_OK        Success!
    *
    * When a file is deleted, the file table entry is marked as invalid yet it still remains.
    * Accumulation of these dead links bloat the file table and can increase the time taken to lookup a
    * file.
    **/
    RES_t ffs_cleanupFT(void);

    /**
    * \brief Returns the number of unused blocks
    * \returns    Number of free blocks
    **/
    uint16_t ffs_blocksFree(void);
///\}

///\name File Operations
///\{

    /**
    * \brief Opens a file for reading or writing.
    * \param    FILE            Pointer to the file object
    * \param    [in] filename        Null-terminated string containing the filename
    * \param    [in] filemode        Mode to open the file (See #FFS_FILEMODE)
    * \retval    RES_PARAMERR    Invalid input parameter
    * \retval    RES_FULL        Storage medium is full (write)
    * \retval    RES_NOTFOUND    File not found (read)
    * \retval    RES_OK            Success!
    *
    **/
    RES_t ffs_fopen(FFS_FILE_t* FILE, char *filename, FFS_FILEMODE_t filemode);

    /**
    * \brief Closes the file
    * \param    FILE            Pointer to the file object
    * \retval    RES_PARAMERR    Invalid input parameter (File may already be closed)
    * \retval    RES_OK            Success!
    *
    **/
    RES_t ffs_fclose(FFS_FILE_t* FILE);

    /**
    * \brief Deletes a file by name
    * \param    filename    Null-terminated string containing the filename
    * \retval    RES_OK    Success
    *
    * If the file is not found, this function succeeds since it wont exist either way.
    **/
    RES_t ffs_remove(char *filename);

    /**
    * \brief Returns the current byte offset from the beginning of the file
    * \param    FILE    Pointer to the file object
    * \return    Byte offset from the beginning of the file
    *
    **/
    uint32_t ffs_ftell(FFS_FILE_t* FILE);

    /**
    * \brief Gets the filename of the next file in the file table
    * \param    [out] filename    Null-terminated string containing the filename
    * \retval    RES_OK    Success. \c filename is valid
    * \retval    RES_END    Reached the end of the file table. \c filename is not valid.
    *
    * Call this function repeatedly to get a list of all the available files.
    * Passing a NULL pointer into filename resets the internal file counter.
    **/
    RES_t ffs_getFile(char *filename);
///\}

///\name Read mode functions
///\{

    /**
    * \brief Reads data from the file
    * \param    FILE    Pointer to the file object
    * \param    [out] data    Pointer to destination buffer
    * \param    [in] size    Number of bytes to read from file
    * \return    Number of bytes successfully read
    *
    **/
    size_t ffs_fread( uint8_t *data, size_t size, FFS_FILE_t* FILE );

    /**
    * \brief Seeks to the specified byte offset from the beginning of the file.
    * \param    FILE    Pointer to the file object
    * \param    [in] offset    Number of bytes from the beginning of the file
    * \retval    RES_PARAMERR    Invalid input parameter
    * \retval    RES_END            Requested offset is greater than the file length. Seek is set to EOF.
    * \retval    RES_OK            Success!
    *
    **/
    RES_t ffs_fseek(FFS_FILE_t* FILE, uint32_t offset);

    /**
    * \brief Test if at the end of a file
    * \param    FILE    Pointer to the file object
    * \retval    Zero    Read pointer is not at the end of the file
    * \retval    Nonzero    Reached the end of the file
    *
    **/
    int ffs_feof(FFS_FILE_t* FILE);
///\}

///\name Write mode functions
///\{

    /**
    * \brief Writes data to the file
    * \param    FILE    Pointer to the file object
    * \param    [in] data    Pointer to source data
    * \param    [in] size    Number of bytes to write to the file
    * \return    Number of bytes successfully written
    *
    **/
    size_t ffs_fwrite( uint8_t *data, size_t size, FFS_FILE_t* FILE );

    /**
    * \brief Secures any written data to the storage medium.
    * \param    FILE    Pointer to the file object
    * \retval    RES_PARAMERR    Invalid input parameter
    * \retval    RES_OK            Success!
    *
    * If data is written to a file and then it is not properly closed or flushed (due to power failure),
    * It is possible that some data at the end of the file may be lost.
    **/
    RES_t ffs_fflush(FFS_FILE_t* FILE);
///\}

#ifdef __cplusplus
}
#endif

#endif
///\}
