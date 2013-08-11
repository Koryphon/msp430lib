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
* \addtogroup MOD_UART UART IO
* \brief Provides basic text IO functions for the MSP430 UART controller
* \author Alex Mykyta 
*
* This module provides basic text input and output functions to the UART. \n
*
* ### MSP430 Processor Families Supported: ###
*   Family  | Supported
*   ------- | ----------
*   1xx     | -
*   2xx     | Yes
*   4xx     | -
*   5xx     | Yes
*   6xx     | Yes
* 
* \todo Add support for the following functions:
*     - RES_t uart_read(void *buf, size_t size)
*     - RES_t uart_write(void *buf, size_t size)
*     - size_t uart_wrcount(void)
*     - void uart_rdflush(void)
*     - void uart_wrflush(void)
* 
* \todo Add support for 1xx and 4xx series
* 
* \{
**/

/**
* \file
* \brief Include file for \ref MOD_UART "UART IO"
* \author Alex Mykyta 
**/

#ifndef __UART_IO_H__
#define __UART_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "uart_io_config.h"

//==================================================================================================
// Function Prototypes
//==================================================================================================

/**
* \brief Initializes the UART controller
* \attention The initialization routine does \e not setup the IO ports!
**/
void uart_init(void);

/**
* \brief Uninitializes the UART controller
**/
void uart_uninit(void);

// RES_t uart_read(void *buf, size_t size);
// RES_t uart_write(void *buf, size_t size);

/**
* \brief Get the number of bytes available to be read
* \return Number of bytes
**/
size_t uart_rdcount(void);
// size_t uart_wrcount(void);

// void uart_rdflush(void);
// void uart_wrflush(void);

/**
* \brief Reads the next character from the UART
* \details If a character is not immediately available, function will block until it receives one.
* \return The next available character
**/
char uart_getc(void);

/**
* \brief Reads in a string of characters until a new-line character ( \c \\n) is received
* 
* - Reads at most n-1 characters from the UART
* - Resulting string is \e always null-terminated
* - If n is zero, a null character is written to str[0], and the function reads and discards
*     characters until a new-line character is received.
* - If n-1 characters have been read, the function continues reading and discarding characters until
*     a new-line character is received.
* - If an entire line is not immediately available, the function will block until it 
*     receives a new-line character.
* 
* \param [out] str Pointer to the destination string buffer
* \param n The size of the string buffer \c str
* \return \c str on success. \c NULL otherwise
**/
char *uart_gets_s(char *str, size_t n);

/**
* \brief Writes a character to the UART
* \param c character to be written
**/
void uart_putc(char c);

/**
* \brief Writes a character string to the UART
* \param s Pointer to the Null-terminated string to be sent
**/
void uart_puts(char *s);

#ifdef __cplusplus
}
#endif

#endif
///\}
