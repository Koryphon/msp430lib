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
* Alex M.       06/07/2011   born
*
*=================================================================================================*/

/**
* \addtogroup MOD_FLASHSPAN
* \{
* \name Implementations
* \{
* \addtogroup MOD_FLASHSPAN_SST25VF Spanned SST25VF Serial Flash Volume
* \brief Implementation of \ref MOD_FLASHSPAN "Spanned Flash Memory Volume" using an
*            array of \ref MOD_SST25VF "SST25VF Serial Flash" devices
* \author Alex Mykyta
*
* Compiling with Cygwin emulates the Spanned Flash Volume on the local filesystem. \n \n
*
* This implementation of \ref MOD_FLASHSPAN "Spanned Flash Memory Volume" also requires the following
* modules:
*    - \ref MOD_SST25VF "SST25VF Serial Flash"
*    - \ref MOD_SPI "SPI Bus"
* \{
**/

/**
* \file
* \brief Code for implementation of \ref MOD_FLASHSPAN "Spanned Flash Memory Volume" with an
*             array of \ref MOD_SST25VF "SST25VF Serial Flash" devices
* \author Alex Mykyta
**/

#include <stdint.h>

#ifdef __MSP430__
#include "SST25VF.h"
#else
#ifdef __CYGWIN32__
#include "SST25VF_W32TB.h"
#endif
#endif

#include "FlashSPAN.h"

flashSPAN_t flashSPAN;

//--------------------------------------------------------------------------------------------------
RES_t flashSPAN_Init(void)
{
    // Check each device and fetch the DeviceBlocks. If any device is unresponsive, return a fail.
    uint8_t i;

    flashSPAN.BlockCount = 0;
    for (i = 0; i < FLASH_DEVICECOUNT; i++) {
        sst25vf_SetCurrentDevice(i);
        switch (sst25vf_Init()) {
#ifdef _SST25VF_W32TB_H_
        case TEST_4000_ID:
            flashSPAN.DeviceBlocks[i] = (TEST_4000_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (TEST_4000_SIZE / FLASH_BLOCKSIZE);
            break;
        case TEST_18000_ID:
            flashSPAN.DeviceBlocks[i] = (TEST_18000_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (TEST_18000_SIZE / FLASH_BLOCKSIZE);
            break;
#endif
        case SST25VF040_ID:
            flashSPAN.DeviceBlocks[i] = (SST25VF040_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (SST25VF040_SIZE / FLASH_BLOCKSIZE);
            break;
        case SST25VF080_ID:
            flashSPAN.DeviceBlocks[i] = (SST25VF080_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (SST25VF080_SIZE / FLASH_BLOCKSIZE);
            break;
        case SST25VF016_ID:
            flashSPAN.DeviceBlocks[i] = (SST25VF016_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (SST25VF016_SIZE / FLASH_BLOCKSIZE);
            break;
        case SST25VF032_ID:
            flashSPAN.DeviceBlocks[i] = (SST25VF032_SIZE / FLASH_BLOCKSIZE);
            flashSPAN.BlockCount += (SST25VF032_SIZE / FLASH_BLOCKSIZE);
            break;
        default:
            return(RES_FAIL);
        }
    }

    return(RES_OK);
}
//--------------------------------------------------------------------------------------------------

RES_t flashSPAN_Read(uint32_t address, uint8_t *data, uint16_t nBytes)
{
    uint8_t device;
    uint32_t maxNbytes;

    // check if start address is valid
    if (address >= ((uint32_t)flashSPAN.BlockCount * FLASH_BLOCKSIZE)) {
        return(RES_PARAMERR); //requested data is past available address space
    }

    // check if access range is within the address range
    if ((address + nBytes) > ((uint32_t)flashSPAN.BlockCount * FLASH_BLOCKSIZE)) {
        return(RES_PARAMERR);
    }


    // Calculate device index and local address
    device = 0;
    while (address >= ((uint32_t)flashSPAN.DeviceBlocks[device]*FLASH_BLOCKSIZE)) {
        address -= ((uint32_t)flashSPAN.DeviceBlocks[device] * FLASH_BLOCKSIZE);
        device++;
    }

    // access addresses per device.
    while (nBytes > 0) {
        // calculate the number of bytes that can be accessed in the current device
        maxNbytes = ((uint32_t)flashSPAN.DeviceBlocks[device] * FLASH_BLOCKSIZE) - address;
        if (nBytes > maxNbytes) {
            // overflows to the next device
            sst25vf_SetCurrentDevice(device);
            sst25vf_Read(address, data, maxNbytes);
            nBytes -= maxNbytes; // decrement the number of bytes accessed
            data += maxNbytes; // increment the data pointer
            address = 0;
            device++;
        }
        else {
            // finish up read
            sst25vf_SetCurrentDevice(device);
            sst25vf_Read(address, data, nBytes);
            break;
        }
    }
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
RES_t flashSPAN_Write(uint32_t address, uint8_t *data, uint16_t nBytes)
{
    uint8_t device;
    uint32_t maxNbytes;

    // check if start address is valid
    if (address >= ((uint32_t)flashSPAN.BlockCount * FLASH_BLOCKSIZE)) {
        return(RES_PARAMERR); //sector is past available address space
    }

    // check if access range is within the address range
    if ((address + nBytes) > ((uint32_t)flashSPAN.BlockCount * FLASH_BLOCKSIZE)) {
        return(RES_PARAMERR);
    }


    // Calculate device index and local address
    device = 0;
    while (address >= ((uint32_t)flashSPAN.DeviceBlocks[device]*FLASH_BLOCKSIZE)) {
        address -= ((uint32_t)flashSPAN.DeviceBlocks[device] * FLASH_BLOCKSIZE);
        device++;
    }

    // access addresses per device.
    while (nBytes > 0) {
        // calculate the number of bytes that can be accessed in the current device
        maxNbytes = ((uint32_t)flashSPAN.DeviceBlocks[device] * FLASH_BLOCKSIZE) - address;
        if (nBytes > maxNbytes) {
            // overflows to the next device
            sst25vf_SetCurrentDevice(device);
            sst25vf_Write(address, data, maxNbytes);
            nBytes -= maxNbytes; // decrement the number of bytes accessed
            data += maxNbytes; // increment the data pointer
            address = 0;
            device++;
        }
        else {
            // finish up write
            sst25vf_SetCurrentDevice(device);
            sst25vf_Write(address, data, nBytes);
            break;
        }
    }
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
RES_t flashSPAN_EraseBlock(uint16_t block)
{
    uint8_t device;

    // check if block is valid
    if (block >= (flashSPAN.BlockCount)) {
        return(RES_PARAMERR); //sector is past available address space
    }

    // Calculate device index and local block
    device = 0;
    while (block >= (flashSPAN.DeviceBlocks[device])) {
        block = block - flashSPAN.DeviceBlocks[device];
        device++;
    }

    // perform block erase
    sst25vf_SetCurrentDevice(device);
#if (FLASH_BLOCKSIZE == 0x1000)
    sst25vf_4kErase(block * FLASH_BLOCKSIZE);
#elif (FLASH_BLOCKSIZE == 0xC800)
    sst25vf_32kErase(block * FLASH_BLOCKSIZE);
#else
#error "Invalid FLASH_BLOCKSIZE"
#endif
    return(RES_OK);
}

//--------------------------------------------------------------------------------------------------
RES_t flashSPAN_EraseAll(void)
{
    uint8_t device;

    for (device = 0; device < FLASH_DEVICECOUNT; device++) {
        sst25vf_SetCurrentDevice(device);
        sst25vf_ChipErase();
    }
    return(RES_OK);
}

///\}
///\}
///\}
