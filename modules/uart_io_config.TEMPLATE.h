/**
* \addtogroup MOD_UART
* \{
**/

/**
* \file
* \brief Configuration include file for \ref MOD_UART
* \author Alex Mykyta 
**/

#ifndef __UART_IO_CONFIG_H__
#define __UART_IO_CONFIG_H__

//==================================================================================================
// UART IO Config
//
// Configuration for: PROJECT_NAME
//==================================================================================================

//==================================================================================================
/// \name Configuration
/// Configuration defines for the \ref MOD_UART module
/// \{
//==================================================================================================

//  ===================================================
//  = NOTE: Actual ports must be configured manually! =
//  ===================================================

/// UART IO Operating Mode
#define UIO_USE_INTERRUPTS    0    ///< \hideinitializer
/**<    0 = Polling Mode    : No buffers used. RX and TX is done within the function calls. \n
*        1 = Interrupt Mode    : All IO is buffered. RX and TX is handled in interrupts.
**/
    
/// RX buffer size (Interrupt mode only)
#define UIO_RXBUF_SIZE    256    ///< \hideinitializer

/// TX buffer size (Interrupt mode only)
#define UIO_TXBUF_SIZE    256    ///< \hideinitializer

/// Select which USCI module to use
#define UIO_USE_DEV        0    ///< \hideinitializer
/**<    0 = USCIA0 \n
*         1 = USCIA1 \n
*         2 = USCIA2 \n
*         3 = USCIA3
**/

/// Select which clock source to use
#define UIO_CLK_SRC        2    ///< \hideinitializer
/**<    0 = External \n
*        1 = ACLK    \n
*        2 = SMCLK
**/


// Baud Rates for SMCLK = 18 MHz
#define UIO_BR0_9600    0x53    ///< \hideinitializer
#define UIO_BR1_9600    0x07    ///< \hideinitializer
#define UIO_MCTL_9600    0x00    ///< \hideinitializer

#define UIO_BR0_19200    0xA9    ///< \hideinitializer
#define UIO_BR1_19200    0x03    ///< \hideinitializer
#define UIO_MCTL_19200    0x55    ///< \hideinitializer

#define UIO_BR0_115200    0x9C    ///< \hideinitializer
#define UIO_BR1_115200    0x00    ///< \hideinitializer
#define UIO_MCTL_115200    0x22    ///< \hideinitializer

// Default Baud Rate
#define UIO_BR0_DEFAULT        UIO_BR0_9600
#define UIO_BR1_DEFAULT        UIO_BR1_9600
#define UIO_MCTL_DEFAULT    UIO_MCTL_9600
///\}
    
#endif
///\}
