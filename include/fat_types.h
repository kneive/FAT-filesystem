#ifndef FAT_TYPES_H
#define FAT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// define FAT TYPE variants

typedef enum {
    FAT_TYPE_FAT12, // volumes < 4085 clusters
    FAT_TYPE_FAT16, // volumes 5085 - 65524 clusters
    FAT_TYPE_FAT32  // volumes > 65524 clusters
} fat_type_t;

// represent cluster numbers
typedef uint32_t cluster_t;

// FAT Entry markers: 0x0000 means cluster is available for allocation
#define FAT_FREE 0x0000


// end of chain markers: indicate last cluster in a cluster chain
#define FAT12_EOC 0x0FF8        // 0x0FF8 - 0x0FFF are all EOC
#define FAT16_EOC 0xFFF8        // 0xFFF8 - 0xFFFF are all EOC
#define FAT32_EOC 0x0FFFFFF8    // 0x0FFFFFF8 - 0x0FFFFFFF are all EOC

// bad cluster markers: indicate physically damaged clusters
#define FAT12_BAD 0x0FF7
#define FAT16_BAD 0xFFF7
#define FAT32_BAD 0x0FFFFFF7

// reserved clusters: cluster 0 and 1 are always reserved and never allocated
#define FAT_RESERVED_CLUSTER_0 0
#define FAT_RESERVED_CLUSTER_1 1

// first data cluster
#define FAT_FIRST_VALID_CLUSTER 2

// file attribute flags: describe file/directory properties in directory entries
#define FAT_ATTR_READ_ONLY 0x01 
#define FAT_ATTR_HIDDEN 0x02    
#define FAT_ATTR_SYSTEM 0x04    // OS file
#define FAT_ATTR_VOLUME_ID 0x08 // volume label
#define FAT_ATTR_DIRECTORY 0x10 // entry is a subdirectory
#define FAT_ATTR_ARCHIVE 0x20   // file modified since last backup
#define FAT_ATTR_LONG_NAME 0x0F

// Error codes: standardized return values for all driver functions

typedef enum {
    FAT_OK = 0,                     // success code

    // parameter validation errors
    FAT_ERR_INVALID_PARAM,          // Null pointer or invalid argument
    FAT_ERR_NO_MEMORY,              // memory allocation failed

    // device I/O errors
    FAT_ERR_DEVICE_ERROR,           // storage device failed

    // volume / filesystem errors
    FAT_ERR_INVALID_BOOT_SECTOR,    // boot sector signature != 0xAA55
    FAT_ERR_UNSUPPORTED_FAT_TYPE,   // not FAT12/16/32

    // File / directory operation errors
    FAT_ERR_NOT_FOUND,              // file / directory does not exist
    FAT_ERR_ALREADY_EXISTS,         // file / directory already exists
    FAT_ERR_NOT_A_DIRECTORY,        // open file as directory
    FAT_ERR_IS_A_DIRECTORY,         // open directory as file
    FAT_ERR_DIRECTORY_NOT_EMPTY,    // delete non-empty directory

    // space allocation errors
    FAT_ERR_DISK_FULL,              // no free clusters available
    FAT_ERR_FILE_TOO_LARGE,         // file exceeds FAT type limits

    // data integrity errors
    FAT_ERR_INVALID_CLUSTER,        // cluster number out of valid range
    FAT_ERR_CORRUPTED,              // filesystem corruption detected

    // access control errors
    FAT_ERR_READ_ONLY,              // write on rd-only file / volume attempted

    // file operation errors
    FAT_ERR_EOF                     // read beyond end of file attempted
} fat_error_t;

// boot sector signature: magic number at bytes 510-511 validate boot sector
#define FAT_BOOT_SIGNATURE 0xAA55

// special values for directory entries: special 1st byte values for dir entries
#define FAT_DIR_ENTRY_FREE 0x00     // entry is never used (end of directory)
#define FAT_DIR_ENTRY_DELETED 0xE5  // entry was deleted (reusable)
#define FAT_DIR_ENTRY_KANJI 0x05    // first byte is actually 0xE5 (kanji)

// seek modes: file seeking modes
#define FAT_SEEK_SET 0 // seek from beginning of a file
#define FAT_SEEK_CUR 1 // seek from current position
#define FAT_SEEK_END 2 // seek from end of file

// file open flags: control how a file is opened
#define FAT_O_RDONLY 0x01   // read only
#define FAT_O_WRONLY 0x02   // write only
#define FAT_O_RDWR 0x03     // read and write
#define FAT_O_CREATE 0x04   // create file if it doesn't exist
#define FAT_O_TRUNC 0x08    // truncate file to 0 length on open

#endif