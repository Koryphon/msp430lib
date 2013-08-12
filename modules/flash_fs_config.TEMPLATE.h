/**
* \addtogroup MOD_FLASHFS
* \{
**/

/**
* \file
* \brief Configuration include file for \ref MOD_FLASHFS "Flash File System"
* \author Alex Mykyta
**/

#ifndef _FLASH_FS_CONFIG_H_
#define _FLASH_FS_CONFIG_H_

//==================================================================================================
/// \name Configuration
/// Configuration defines for the \ref MOD_FLASHFS module
/// \{
//==================================================================================================

#define FFS_FILENAME_LEN    14        ///< Max filename length in characters

/// Value that the flash erases to
#define FFS_ERASE_VAL        0xFF
/**<
 *    0xFF - Erases to all '1's \n
 *    0x00 - Erases to all '0's
**/

#define FFS_CLEANUP_FT_MODE    0
/**<
 *    0 - Use local buffer (Faster but requires FLASH_BLOCKSIZE bytes of RAM) \n
 *    1 - Use a temporary scratchpad block in FLASH (Slower and wears down FLASH more)
**/

///\}

#endif
///\}
