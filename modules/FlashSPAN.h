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
* \addtogroup MOD_FLASHSPAN Spanned Flash Memory Volume
* \brief Creates a spanned flash volume out of several devices
* \author Alex Mykyta
*
* This module merges several similar smaller Flash memories into one large virtual device with a
* continuous address space. This module defines a generic interface which can implement a
* variety of memory devices.
*
* \{
**/

/**
* \file
* \brief Generic include file for \ref MOD_FLASHSPAN "Spanned Flash Memory Volume"
* \author Alex Mykyta
**/

#ifndef _FLASHSPAN_H_
#define _FLASHSPAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <result.h>
#include "FlashSPAN_config.h"

///\brief Data object that stores information about the spanned flash volume.
    typedef struct {
        uint16_t DeviceBlocks[FLASH_DEVICECOUNT]; ///< Array of block counts for each device
        uint16_t BlockCount; ///< Total volume block count
    } flashSPAN_t;

///\brief flashSPAN object is externally accessible for higher level modules
    extern flashSPAN_t flashSPAN;

    /**
    * \brief Initializes the spanned flash volume and populates the #flashSPAN object
    * \retval RES_OK
    * \retval RES_FAIL
    * \attention The initialization routine does \e not setup the IO ports!
    **/
    RES_t flashSPAN_Init(void);

    /**
    * \brief Read data from the spanned flash volume
    * \param [in] address Start address of read operation
    * \param [in] nBytes Number of bytes to be read
    * \param [out] data Data read
    * \retval RES_OK
    * \retval RES_PARAMERR Invalid address range
    **/
    RES_t flashSPAN_Read(uint32_t address, uint8_t *data, uint16_t nBytes);

    /**
    * \brief Write data to the Flash volume
    * \param [in] address Start address of write operation
    * \param [in] nBytes Number of bytes to be written
    * \param [in] data Data to be written
    * \retval RES_OK
    * \retval RES_PARAMERR Invalid address range
    **/
    RES_t flashSPAN_Write(uint32_t address, uint8_t *data, uint16_t nBytes);

    /**
    * \brief Erase a block of Flash memory
    * \param [in] block Block number
    * \retval RES_OK
    * \retval RES_PARAMERR Invalid address range
    **/
    RES_t flashSPAN_EraseBlock(uint16_t block);

    /**
    * \brief Erase the entire spanned flash volume
    * \retval RES_OK
    **/
    RES_t flashSPAN_EraseAll(void);

#ifdef __cplusplus
}
#endif

#endif
///\}
