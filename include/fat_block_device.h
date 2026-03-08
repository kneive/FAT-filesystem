#ifndef FAT_BLOCK_DEVICE_H
#define FAT_BLOCK_DEVICE_H

#include <stdint.h>

// block device interface: abstraction from underlying storage device

typedef struct {
    int(*read_sectors)(void *device, uint32_t sector, uint32_t count, 
                       void *buffer);
    int(*write_sectors)(void *device, uint32_t sector, uint32_t count, 
                        const void *buffer);
    int (*get_sector_count)(void *device, uint32_t *sector_count);
    int (*get_sector_size)(void *device, uint32_t *sector_size);
    void *device_data;
} fat_block_device_t;

// sector size
#define FAT_SECTOR_SIZE 512

fat_block_device_t *fat_block_device_file_create(const char *filename, 
                                                 uint32_t sector_count);

fat_block_device_t *fat_block_device_memory_create(uint32_t sector_count);

void fat_block_device_destroy(fat_block_device_t *device);

#endif