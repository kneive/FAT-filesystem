// file based block device
#include "fat_block_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// file based block device

typedef struct {
    FILE *file;
    uint32_t sector_count;
    uint32_t sector_size;
} file_block_device_t;

static int file_read_sectors(void *device, 
                             uint32_t sector, 
                             uint32_t count, 
                             void *buffer){

    file_block_device_t *dev = (file_block_device_t*) device;

    // seek sector position
    if(fseek(dev->file, sector * dev->sector_size, SEEK_SET) != 0){
        return -1;
    }

    size_t bytes_read = fread(buffer, dev->sector_size, count, dev->file);
    return (bytes_read == count) ? 0 : -1;
}

static int file_write_sectors(void *device, 
                              uint32_t sector, 
                              uint32_t count, 
                              const void *buffer){

    file_block_device_t *dev = (file_block_device_t*)device;

    // seek sector position
    if(fseek(dev->file, sector * dev->sector_size, SEEK_SET) != 0){
        return -1;
    }

    size_t bytes_written = fwrite(buffer, dev->sector_size, count, dev->file);
    fflush(dev->file);
    return (bytes_written == count) ? 0 : -1;
}

static int file_get_sector_count(void *device, uint32_t *sector_count){

    file_block_device_t *dev = (file_block_device_t*)device;
    *sector_count = dev->sector_count;
    return 0;
}

static int file_get_sector_size(void *device, uint32_t *sector_size){

    file_block_device_t *dev = (file_block_device_t*)device;
    *sector_size = dev->sector_size;
    return 0;
}

fat_block_device_t * fat_block_device_file_create(const char *filename, 
                                                  uint32_t sector_count){

    file_block_device_t *dev = malloc(sizeof(file_block_device_t));
    if(!dev){
        return NULL;
    }

    dev->file = fopen(filename, "r+b");
    if(!dev->file){
        dev->file = fopen(filename, "w+b");
    }

    dev->sector_count = sector_count;
    dev->sector_size = FAT_SECTOR_SIZE;

    fat_block_device_t *block_dev = malloc(sizeof(fat_block_device_t));
    block_dev->read_sectors = file_read_sectors;
    block_dev->write_sectors = file_write_sectors;
    block_dev->get_sector_count = file_get_sector_count;
    block_dev->get_sector_size = file_get_sector_size;
    block_dev->device_data = dev;

    return block_dev;
}

// memory based block device
typedef struct {
    uint8_t *memory;
    uint32_t sector_count;
    uint32_t sector_size;
} memory_block_device_t;

static int memory_read_sectors(void *device, 
                               uint32_t sector, 
                               uint32_t count, 
                               void *buffer){

    memory_block_device_t *dev = (memory_block_device_t*)device;

    if(sector + count > dev->sector_count){
        return -1;
    }

    memcpy(buffer, 
            dev->memory + (sector * dev->sector_size), 
            count * dev->sector_size);
    
    return 0;
}

static int memory_write_sectors(void *device, 
                                uint32_t sector, 
                                uint32_t count, 
                                const void *buffer){

    memory_block_device_t *dev = (memory_block_device_t*)device;

    if(sector + count > dev->sector_count){
        return -1;
    }

    memcpy(dev->memory + (sector * dev->sector_size), 
           buffer, 
           count * dev->sector_size);
    
    return 0;
}

static int memory_get_sector_count(void* device, uint32_t *sector_count){

    memory_block_device_t *dev = (memory_block_device_t*)device;
    *sector_count = dev->sector_count;
    return 0;
}

static int memory_get_sector_size(void* device, uint32_t *sector_size){

    memory_block_device_t *dev = (memory_block_device_t*)device;
    *sector_size = dev->sector_size;
    return 0;
}


fat_block_device_t *fat_block_device_memory_create(uint32_t sector_count){

    memory_block_device_t *dev = malloc(sizeof(memory_block_device_t));
    dev->memory = calloc(sector_count, FAT_SECTOR_SIZE);
    dev->sector_count = sector_count;
    dev->sector_size = FAT_SECTOR_SIZE;

    fat_block_device_t *block_dev = malloc(sizeof(fat_block_device_t));
    block_dev->read_sectors = memory_read_sectors;
    block_dev->write_sectors = memory_write_sectors;
    block_dev->get_sector_count = memory_get_sector_count;
    block_dev->get_sector_size = memory_get_sector_size;
    block_dev->device_data = dev;

    return block_dev;
}