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
* Alex M.       2010-11-15   born
* Alex M.       2013-02-10   Renamed functions to be consistent with generic string IO format
* Alex M.       2013-08-10   Added uninit routine
*
*=================================================================================================*/

/**
* \addtogroup MOD_UART
* \{
**/

/**
* \file
* \brief Code for \ref MOD_UART "UART IO"
* \author Alex Mykyta
**/

#include <stdint.h>
#include <stddef.h>

#include <msp430_xc.h>
#include <result.h>
#include "uart_io.h"
#include "uart_io_internal.h"
#include "fifo.h"

///\cond INTERNAL
#if (UIO_USE_INTERRUPTS == 1)
static char rxbuf[UIO_RXBUF_SIZE];
static FIFO_t RXFIFO;
static char txbuf[UIO_TXBUF_SIZE];
static FIFO_t TXFIFO;

static uint8_t txbusy;
#endif
///\endcond


//==================================================================================================
// Hardware Abstraction Layer
//==================================================================================================
void uart_init(void)
{
#if defined(__MSP430_HAS_USCI__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    UIO_CTL1 |= UCSWRST; // soft reset
    UIO_CTL0 = 0;
    UIO_CTL1 = (UIO_CLK_SRC * 64) + UCSWRST;
    UIO_BR0  = UIO_BR0_DEFAULT;
    UIO_BR1  = UIO_BR1_DEFAULT;
    UIO_MCTL = UIO_MCTL_DEFAULT;

    UIO_CTL1 &= ~UCSWRST;

#if (UIO_USE_INTERRUPTS == 1)
    fifo_init(&RXFIFO, rxbuf, UIO_RXBUF_SIZE);
    fifo_init(&TXFIFO, txbuf, UIO_TXBUF_SIZE);
    txbusy = 0;
#if (UIO_ISR_SPLIT == 0)
    UIO_IE = UCRXIE;
#else
    UIO_IE = UIO_UCARXIE;
#endif
#endif

#elif defined(__MSP430_HAS_UCA__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#error "Hardware Abstraction for UCA not written yet!"
#endif
}

//--------------------------------------------------------------------------------------------------
void uart_uninit(void)
{
#if defined(__MSP430_HAS_USCI__)
    UIO_CTL1 |= UCSWRST;
#endif
}

//--------------------------------------------------------------------------------------------------
char uart_getc(void)
{
#if defined(__MSP430_HAS_USCI__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if (UIO_USE_INTERRUPTS == 1)
    char chr;
    while (fifo_rdcount(&RXFIFO) == 0); // wait until char recieved
    fifo_read(&RXFIFO, &chr, 1);
    return(chr);
#else
    while ((UIO_IFG & UCRXIFG) == 0); // wait until char recieved
    return(UIO_RXBUF);
#endif

#elif defined(__MSP430_HAS_UCA__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#error "Hardware Abstraction for UCA not written yet!"
#endif
}

//--------------------------------------------------------------------------------------------------
size_t uart_rdcount(void)
{
#if defined(__MSP430_HAS_USCI__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if (UIO_USE_INTERRUPTS == 1)
    return(fifo_rdcount(&RXFIFO));
#else
    if ((UIO_IFG & UCRXIFG) != 0)
    {
        return(1);
    }
    else
    {
        return(0);
    }
#endif

#elif defined(__MSP430_HAS_UCA__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#error "Hardware Abstraction for UCA not written yet!"
#endif
}

//--------------------------------------------------------------------------------------------------
void uart_putc(char c)
{
#if defined(__MSP430_HAS_USCI__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if (UIO_USE_INTERRUPTS == 1)
    char chr;
    while (fifo_wrcount(&TXFIFO) == 0); // wait until fifo has room for another
    fifo_write(&TXFIFO, &c, 1);

    if (txbusy == 0)
    {
        txbusy = 1;
        fifo_read(&TXFIFO, &chr, 1);
        UIO_TXBUF = chr;
#if (UIO_ISR_SPLIT == 0)
        UIO_IE |= UCTXIE;
#else
        UIO_IE |= UIO_UCATXIE;
#endif

    }
#else
    while ((UIO_IFG & UCTXIFG) == 0); // wait until txbuf is empty
    UIO_TXBUF = c;
#endif

#elif defined(__MSP430_HAS_UCA__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#error "Hardware Abstraction for UCA not written yet!"
#endif
}

//--------------------------------------------------------------------------------------------------

///\cond INTERNAL
#if defined(__MSP430_HAS_USCI__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if (UIO_USE_INTERRUPTS == 1)
#if (UIO_ISR_SPLIT == 0)
// RX/TX Interrupt Service Routine
ISR(UIO_ISR_VECTOR)
{

    uint16_t iv = UIO_IV;
    char chr;
    if (iv == 0x02)
    {
        // Data Recieved
        chr = UIO_RXBUF;
        fifo_write(&RXFIFO, &chr, 1);
    }
    else if (iv == 0x04)
    {
        // Transmit Buffer Empty
        if (fifo_read(&TXFIFO, &chr, 1) == RES_OK)
        {
            UIO_TXBUF = chr;
        }
        else
        {
            txbusy = 0;
            UIO_IE &= ~UCTXIE; // disable tx interrupt
        }
    }
}
#else
// RX Interrupt Service Routine
ISR(UIO_RXISR_VECTOR)
{
    char chr;
    chr = UIO_RXBUF;
    fifo_write(&RXFIFO, &chr, 1);
}

// TX Interrupt Service Routine
ISR(UIO_TXISR_VECTOR)
{
    char chr;
    if (fifo_read(&TXFIFO, &chr, 1) == RES_OK)
    {
        UIO_TXBUF = chr;
    }
    else
    {
        txbusy = 0;
        UIO_IE &= ~UIO_UCATXIE; // disable tx interrupt
    }
}
#endif
#endif
#elif defined(__MSP430_HAS_UCA__)    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#error "Hardware Abstraction for UCA not written yet!"
#endif

///\endcond

//==================================================================================================
//   General functions
//==================================================================================================
void uart_puts(char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

//--------------------------------------------------------------------------------------------------
char *uart_gets_s(char *str, size_t n)
{
    char c;
    size_t idx = 0;

    // write chars to buffer
    while (idx < (n - 1))
    {
        c = uart_getc();
        if (c == '\n')
        {
            str[idx] = 0;
            return(str);
        }
        else
        {
            str[idx] = c;
            idx++;
        }
    }

    str[idx] = 0;

    // discard chars
    while (1)
    {
        c = uart_getc();
        if (c == '\n')
        {
            return(str);
        }
    }
}


///\}
