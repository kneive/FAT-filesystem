#ifndef FAT_BOOT_H
#define FAT_BOOT_H

#include <stdint.h>
#include "fat_types.h"
#include "fat_block_device.h"

// boot sector structure: attributes must match on disk layout (->packed)
typedef struct __attribute__((packed)) {
    
    // common BPB fields
    uint8_t jmp_boot[3];
    uint8_t oem_name[8];

    // bios parameter block: core filesystem parameters
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // extended BPB
    union{
        
        // FAT12/16
        struct __attribute__((packed)) {
            uint8_t drive_number;
            uint8_t reserved1;
            uint8_t boot_signature;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat16;

        // FAT32
        struct __attribute__((packed)) {
            uint32_t fat_size_32;
            uint16_t ext_flags;
            uint16_t fs_version;
            uint32_t root_cluster;
            uint16_t fs_info;
            uint16_t backup_boot_sector;
            uint8_t reserved[12];
            uint8_t drive_number;
            uint8_t reserved1;
            uint8_t boot_signature;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat32;
    } extended;

    // Boot code: FAT32 - bytes 90-509 FAT16 - bytes 62-509
} fat_boot_sector_t;

fat_error_t fat_parse_boot_sector(fat_block_device_t *device, 
                                  fat_boot_sector_t *boot_sector);

fat_error_t fat_determine_type(const fat_boot_sector_t *boot_sector, 
                               fat_type_t *type);

fat_error_t fat_calculate_data_region(const fat_boot_sector_t *boot_sector, 
                                      uint32_t *first_data_sector);

#endif