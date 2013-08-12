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
* \addtogroup MOD_SST25VF SST25VF Serial Flash
* \brief Interface to SST's SPI Flash devices
* \author Alex Mykyta
* \{
**/

/**
* \file
* \brief Include file for \ref MOD_SST25VF "SST25VF Serial Flash"
* \author Alex Mykyta
* This module also requires the following module:
*    - \ref MOD_SPI "SPI Bus"
*
* \todo Clean this module up. Most of the functions & defines in the header can be static/internal.
*
*
**/

#ifndef _SST25VF_H_
#define _SST25VF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SST25VF_config.h"

//==================================================================================================
// Constant Definitions
//==================================================================================================

/// \name Status Register Bits
///\{
#define SST_BUSY    0x01
#define SST_WEL        0x02
#define SST_BP0        0x04
#define SST_BP1        0x08
#define SST_BP2        0x10
#define SST_BP3        0x20
#define SST_AAI        0x40
#define SST_BPL        0x80
///\}

/// \name Commands
///\{
#define SST_RD          0x03
#define SST_ERASE4k     0x20
#define SST_ERASE32k    0x52
#define SST_ERASE64k    0xD8
#define SST_CHIPERASE   0x60
#define SST_WRBYTE      0x02
#define SST_WRAAI       0xAD
#define SST_RDSR        0x05
#define SST_EWSR        0x50
#define SST_WRSR        0x01
#define SST_WREN        0x06
#define SST_WRDI        0x04
#define SST_RDID        0x90
#define SST_JEDECID     0x9F
#define SST_EBSY        0x70
#define SST_DBSY        0x80
///\}

/// \name Device IDs
///\{
#define SST_INVALID_ID      0x0000
#define SST25VF040_ID       0xBF8D
#define SST25VF080_ID       0xBF8E
#define SST25VF016_ID       0xBF41
#define SST25VF032_ID       0xBF4A
///\}

/// \name Device JEDEC IDs
///\{
#define SST25VF040_JEDEC    0x00BF258DL
#define SST25VF080_JEDEC    0x00BF258EL
#define SST25VF016_JEDEC    0x00BF2541L
#define SST25VF032_JEDEC    0x00BF254AL
///\}

/// \name Device Sizes
///\{
#define SST25VF040_SIZE     0x00080000L
#define SST25VF080_SIZE     0x00100000L
#define SST25VF016_SIZE     0x00200000L
#define SST25VF032_SIZE     0x00400000L
///\}
//==================================================================================================
// Function Prototypes
//==================================================================================================

///\name Functions
///\brief Functions closely mirror the SST25VF command set. See device documentation for
/// more detalis.
///\{

    ///\cond PRIVATE
    void sst25vf_xErase(uint32_t Addr, uint8_t EraseCode);
    ///\endcond

    void sst25vf_SetCurrentDevice(uint8_t device);
    uint8_t sst25vf_GetCurrentDevice(void);

    uint16_t sst25vf_Init(void);
    uint8_t sst25vf_RDSR(void);
    void sst25vf_WRSR(uint8_t status);
    void sst25vf_StallBusy(void);
    void sst25vf_CMD(uint8_t data);
#define sst25vf_WREN()            sst25vf_CMD(SST_WREN)
#define sst25vf_WRDI()            sst25vf_CMD(SST_WRDI)
#define sst25vf_EWSR()            sst25vf_CMD(SST_EWSR)
#define sst25vf_EBSY()            sst25vf_CMD(SST_EBSY)
#define sst25vf_DBSY()            sst25vf_CMD(SST_DBSY)
    void sst25vf_ChipErase(void);
    void sst25vf_Read(uint32_t startAddr, uint8_t *data, uint16_t nBytes);
    void sst25vf_WriteByte(uint32_t startAddr, const uint8_t data);
    void sst25vf_Write(uint32_t startAddr, const uint8_t *data, uint16_t nBytes);
    void sst25vf_WriteSlow(uint32_t startAddr, const uint8_t *data, uint16_t nBytes);
    void sst25vf_AAIStart(uint32_t startAddr, const uint8_t D0, const uint8_t D1);
    void sst25vf_AAICont(const uint8_t D0, const uint8_t D1);
#define sst25vf_4kErase(A)        sst25vf_xErase((A),SST_ERASE4k)
#define sst25vf_32kErase(A)        sst25vf_xErase((A),SST_ERASE32k)
#define sst25vf_64kErase(A)        sst25vf_xErase((A),SST_ERASE64k)
    uint16_t sst25vf_RDID();
    uint32_t sst25vf_JEDECID();
///\}

#ifdef __cplusplus
}
#endif

#endif
///\}
