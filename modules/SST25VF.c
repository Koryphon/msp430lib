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
* Alex M.       2011-04-07   born
*
*=================================================================================================*/

/**
* \addtogroup MOD_SST25VF
* \{
**/

/**
* \file
* \brief Code for \ref MOD_SST25VF "SST25VF Serial Flash"
* \author Alex Mykyta
**/

#include <stdint.h>

#include "msp430_xc.h"
#include "SST25VF.h"
#include "spi.h"

//==================================================================================================
// Internal Functions
//==================================================================================================
///\cond INTERNAL

static uint8_t CurrentDevice; // represents the current selected device

#if SST_CE_MODE == 0
#define SST_CE_DEVMASK    (SST_CE_DEV0_BIT|SST_CE_DEV1_BIT|SST_CE_DEV2_BIT|SST_CE_DEV3_BIT|\
                            SST_CE_DEV4_BIT|SST_CE_DEV5_BIT|SST_CE_DEV6_BIT|SST_CE_DEV7_BIT)

static uint8_t CE_Mask;
static const uint8_t CE_MAP[] = {
    SST_CE_DEV0_BIT,
    SST_CE_DEV1_BIT,
    SST_CE_DEV2_BIT,
    SST_CE_DEV3_BIT,
    SST_CE_DEV4_BIT,
    SST_CE_DEV5_BIT,
    SST_CE_DEV6_BIT,
    SST_CE_DEV7_BIT
};
#else
#if(SST_ADDR_WIDTH == 2)
#define ADDR_MASK    (0x03)
#elif(SST_ADDR_WIDTH == 3)
#define ADDR_MASK    (0x07)
#elif(SST_ADDR_WIDTH == 4)
#define ADDR_MASK    (0x0F)
#elif(SST_ADDR_WIDTH == 5)
#define ADDR_MASK    (0x1F)
#elif(SST_ADDR_WIDTH == 6)
#define ADDR_MASK    (0x3F)
#elif(SST_ADDR_WIDTH == 7)
#define ADDR_MASK    (0x7F)
#elif(SST_ADDR_WIDTH == 8)
#define ADDR_MASK    (0xFF)
#else
#error "Invalid SST_ADDR_WIDTH"
#endif
#endif

static void SendAddr(uint32_t addr)
{
    spiSendByte((addr & 0xFF0000) >> 16);
    spiSendByte((addr & 0xFF00) >> 8);
    spiSendByte(addr & 0xFF);
}

//--------------------------------------------------------------------------------------------------
static void sst_CE(void)
{
#if SST_CE_MODE == 0
    SST_CE_POUT &= ~CE_Mask;
#else
    SST_ADDR_EN_POUT &= ~SST_ADDR_EN_BIT;
#endif
}

//--------------------------------------------------------------------------------------------------
static void sst_nCE(void)
{
#if SST_CE_MODE == 0
    SST_CE_POUT |= SST_CE_DEVMASK;
#else
    SST_ADDR_EN_POUT |= SST_ADDR_EN_BIT;
#endif
}

///\endcond
//==================================================================================================
// Functions
//==================================================================================================

void sst25vf_SetCurrentDevice(uint8_t device)
{
    CurrentDevice = device;
#if SST_CE_MODE == 0
    CE_Mask = CE_MAP[device];
#else
    SST_ADDR_POUT &= ~ADDR_MASK;
    SST_ADDR_POUT |= (device & ADDR_MASK)
#endif
}

//--------------------------------------------------------------------------------------------------
uint8_t sst25vf_GetCurrentDevice(void)
{
    return(CurrentDevice);
}

//--------------------------------------------------------------------------------------------------
/**
* \brief Initializes the SPI controller and attempts to identify the device.
* \return SST25VF Device ID
* \attention The initialization routine does \e not setup the IO ports!
**/
uint16_t sst25vf_Init(void)
{
    uint16_t id;

    // Init SPI
    spiInit(SPI_MODE0);

    // attempt to ID device
    id = sst25vf_RDID();
    switch (id) {
    case SST25VF040_ID:
        break;
    case SST25VF080_ID:
        break;
    case SST25VF016_ID:
        break;
    case SST25VF032_ID:
        break;
    default:
        return(SST_INVALID_ID);
    }
    sst25vf_WRSR(0x00);
    sst25vf_DBSY();

    return(id);
}

//--------------------------------------------------------------------------------------------------
uint8_t sst25vf_RDSR(void)
{
    uint8_t result;
    sst_CE();
    spiSendByte(SST_RDSR);
    result = spiGetByte();
    sst_nCE();
    return(result);
}

//--------------------------------------------------------------------------------------------------
void sst25vf_WRSR(uint8_t status)
{
    sst25vf_EWSR();
    sst_CE();
    spiSendByte(SST_WRSR);
    spiSendByte(status);
    sst_nCE();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_StallBusy(void)
{
    while ((sst25vf_RDSR() & SST_BUSY) != 0);
}

//--------------------------------------------------------------------------------------------------
void sst25vf_CMD(uint8_t data)
{
    sst_CE();
    spiSendByte(data);
    sst_nCE();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_Read(uint32_t startAddr, uint8_t *data, uint16_t nBytes)
{
    uint16_t i;
    sst_CE();
    spiSendByte(SST_RD);
    SendAddr(startAddr);
    for (i = 0; i < nBytes; i++) {
        data[i] = spiGetByte();
    }
    sst_nCE();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_WriteSlow(uint32_t startAddr, const uint8_t *data, uint16_t nBytes)
{
    uint16_t i;
    for (i = 0; i < nBytes; i++) {
        sst25vf_WriteByte(startAddr + i, data[i]);
    }
}

//--------------------------------------------------------------------------------------------------
void sst25vf_WriteByte(uint32_t startAddr, const uint8_t data)
{
    sst25vf_WREN();
    sst_CE();
    spiSendByte(SST_WRBYTE);
    SendAddr(startAddr);
    spiSendByte(data);
    sst_nCE();
    sst25vf_StallBusy();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_Write(uint32_t startAddr, const uint8_t *data, uint16_t nBytes)
{
    uint16_t i;

    // If odd start address
    if (startAddr & 0x01) {
        // write 1 byte
        sst25vf_WriteByte(startAddr, data[0]);
        startAddr++;
        nBytes--;
        data++;
    }

    if (nBytes == 0) {
        return; // only one byte. End
    }

    i = 0;
    // Write pairs of bytes aligned to even addresses (AAI)
    if (nBytes >= 2) {
        sst25vf_AAIStart(startAddr, data[i], data[i + 1]);
        i += 2;
        nBytes -= 2;
        sst25vf_StallBusy();

        while (nBytes >= 2) {
            sst25vf_AAICont(data[i], data[i + 1]);
            i += 2;
            nBytes -= 2;
            sst25vf_StallBusy();
        }
        sst25vf_WRDI();
        sst25vf_StallBusy();
    }

    // if one byte remaining
    if (nBytes) {
        // write 1 byte
        startAddr += i;
        sst25vf_WriteByte(startAddr, data[i]);
    }
}

//--------------------------------------------------------------------------------------------------
void sst25vf_AAIStart(uint32_t startAddr, const uint8_t D0, const uint8_t D1)
{
    sst25vf_WREN();
    sst_CE();
    spiSendByte(SST_WRAAI);
    SendAddr(startAddr);
    spiSendByte(D0);
    spiSendByte(D1);
    sst_nCE();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_AAICont(const uint8_t D0, const uint8_t D1)
{
    sst_CE();
    spiSendByte(SST_WRAAI);
    spiSendByte(D0);
    spiSendByte(D1);
    sst_nCE();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_ChipErase(void)
{
    sst25vf_WREN();
    sst_CE();
    spiSendByte(SST_CHIPERASE);
    sst_nCE();
    sst25vf_StallBusy();
}

//--------------------------------------------------------------------------------------------------
void sst25vf_xErase(uint32_t Addr, uint8_t EraseCode)
{
    sst25vf_WREN();
    sst_CE();
    spiSendByte(EraseCode);
    SendAddr(Addr);
    sst_nCE();
    sst25vf_StallBusy();
}

//--------------------------------------------------------------------------------------------------
uint16_t sst25vf_RDID()
{
    uint16_t result;
    sst_CE();
    spiSendByte(SST_RDID);
    spiSendByte(0x00);
    spiSendByte(0x00);
    spiSendByte(0x00);
    result = spiGetByte();
    result <<= 8;
    result |= spiGetByte();
    sst_nCE();
    return(result);
}

//--------------------------------------------------------------------------------------------------
uint32_t sst25vf_JEDECID()
{
    uint32_t result;
    sst_CE();
    spiSendByte(SST_JEDECID);
    result = spiGetByte();
    result <<= 8;
    result |= spiGetByte();
    result <<= 8;
    result |= spiGetByte();
    sst_nCE();
    return(result);
}

///\}
