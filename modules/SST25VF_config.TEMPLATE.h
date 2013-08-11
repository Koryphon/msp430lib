/**
* \addtogroup MOD_SST25VF
* \{
**/

/**
* \file
* \brief Configuration include file for \ref MOD_SST25VF
* \author Alex Mykyta
**/

#ifndef _SST25VF_CONFIG_H_
#define _SST25VF_CONFIG_H_

//==================================================================================================
// SST25VF Config
//
// Configuration for: PROJECT_NAME
//==================================================================================================
//==================================================================================================
/// \name Configuration
/// Configuration defines for the \ref MOD_SST25VF module
/// \{
//==================================================================================================

/// Chip Enable Mode
#define SST_CE_MODE         0       ///< \hideinitializer
/**<    0 = One-hot mode: Each chip's CE line is directly connected to a port \n
*       1 = Addressed chip mode: Array of chips is addressed using 74x138, 74x154 or similar
**/


//--------------------------------------------------------------------------------------------------
// One-hot CE mode (SST_CE_MODE == 0)
//--------------------------------------------------------------------------------------------------
// bit locations of each CE for each SST chip
#define SST_CE_DEV0_BIT     BIT6
#define SST_CE_DEV1_BIT     BIT7
#define SST_CE_DEV2_BIT     0    // disabled
#define SST_CE_DEV3_BIT     0    // disabled
#define SST_CE_DEV4_BIT     0    // disabled
#define SST_CE_DEV5_BIT     0    // disabled
#define SST_CE_DEV6_BIT     0    // disabled
#define SST_CE_DEV7_BIT     0    // disabled

/// CE output port where the CE pins are located
#define SST_CE_POUT         P1OUT

//--------------------------------------------------------------------------------------------------
// Addressed CE Mode (SST_CE_MODE == 1)
//--------------------------------------------------------------------------------------------------
/// Bit location of the address enable bit (Active Low)
#define SST_ADDR_EN_BIT     BIT7    ///< \hideinitializer

/// Output port where the address enable bit is located
#define SST_ADDR_EN_POUT    P1OUT   ///< \hideinitializer

/// Output port where address lines are connected (LSb of address must start at LSb of port)
#define SST_ADDR_POUT       P1OUT   ///< \hideinitializer

/// Bit width of the address lines
#define SST_ADDR_WIDTH      2       ///< \hideinitializer

///\}

#endif

///\}
