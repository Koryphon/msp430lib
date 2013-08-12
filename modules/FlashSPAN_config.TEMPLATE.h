/**
* \addtogroup MOD_FLASHSPAN
* \{
**/

/**
* \file
* \brief Configuration include for \ref MOD_FLASHSPAN "Spanned Flash Memory Volume"
* \author Alex Mykyta
**/

#ifndef _FLASHSPAN_CONFIG_H_
#define _FLASHSPAN_CONFIG_H_

//==================================================================================================
/** \name Configuration
 * Generic configuration defines for the \ref MOD_FLASHSPAN module
 *
 * \details
 *    - The total number of blocks can not exceed \c 0xFFFF
 *    - The total number of bytes can not exceed  \c 0xFFFFFFFF (4 GB)
 *
 * \{
**/
//==================================================================================================

/// Erase block size in bytes. Each device must have equal block sizes.
#define FLASH_BLOCKSIZE        0x1000 ///< \hideinitializer

/// Total number of devices in the volume
#define FLASH_DEVICECOUNT        2 ///< \hideinitializer

///\}

#endif
///\}
