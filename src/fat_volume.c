#include "fat_volume.h"
#include <stdlib.h>
#include <string.h>

fat_error_t fat_mount(fat_block_device_t *device, fat_volume_t *volume){

    // parameter validation
    if(!device || !volume){
        return FAT_ERR_INVALID_PARAM;
    }

    // initialize volume structure to 0 (all fields start in a known state)
    memset(volume, 0, sizeof(fat_volume_t));

    volume->device = device;

    // parse boot sector
    fat_error_t err = fat_parse_boot_sector(device, &volume->boot_sector);
    if(err != FAT_OK){
        return err;
    }

    // determine FAT type
    err = fat_determine_type(&volume->boot_sector, &volume->type);
    if(err != FAT_OK){
        return err;
    }

    // cache frequently-used parameters
    volume->bytes_per_sector = volume->boot_sector.bytes_per_sector;
    volume->sectors_per_cluster = volume->boot_sector.sectors_per_cluster;
    volume->bytes_per_cluster = volume->bytes_per_sector * 
                                    volume->sectors_per_cluster;
    volume->reserved_sector_count = volume->boot_sector.reserved_sector_count;
    volume->num_fats = volume->boot_sector.num_fats;
    volume->root_entry_count = volume->boot_sector.root_entry_count;

    // determine FAT size
    if(volume->boot_sector.fat_size_16 != 0){
        volume->fat_size_sectors = volume->boot_sector.fat_size_16;
    } else {
        volume->fat_size_sectors = volume->boot_sector.extended.fat32.fat_size_32;
    }

    // determine total sectors
    if(volume->boot_sector.total_sectors_16 != 0){
        volume->total_sectors = volume->boot_sector.total_sectors_16;
    } else {
        volume->total_sectors = volume->boot_sector.total_sectors_32;
    }

    // calculate root directory size
    volume->root_dir_sectors = ((volume->root_entry_count * 32) +
                                (volume->bytes_per_sector - 1)) /
                                volume->bytes_per_sector;
    
    // extract cluster for FAT32 root directory
    if(volume->type == FAT_TYPE_FAT32){
        volume->root_cluster = volume->boot_sector.extended.fat32.root_cluster;
    } else {
        volume->root_cluster = 0;
    }

    // calculate important sector offsets
    volume->fat_begin_sector = volume->reserved_sector_count;
    volume->data_begin_sector = volume->reserved_sector_count +
                                (volume->num_fats * volume->fat_size_sectors) +
                                    volume->root_dir_sectors;
    
    uint32_t data_sectors = volume->total_sectors - volume->data_begin_sector;
    volume->total_clusters = data_sectors / volume->sectors_per_cluster;
    
    // allocate and load FAT cache
    volume->fat_cache_size = volume->fat_size_sectors * volume->bytes_per_sector;
    volume->fat_cache = (uint8_t*)malloc(volume->fat_cache_size);
    if(!volume->fat_cache){
        return FAT_ERR_NO_MEMORY;
    }

    int result = device->read_sectors(device->device_data, 
                                      volume->fat_begin_sector, 
                                      volume->fat_size_sectors, 
                                      volume->fat_cache);
    
    if(result != 0){
        free(volume->fat_cache);
        volume->fat_cache = NULL;
        return FAT_ERR_DEVICE_ERROR;
    }

    volume->fat_dirty = false;

    return FAT_OK;
}

fat_error_t fat_flush(fat_volume_t *volume){

    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    // check if the FAT is dirty
    if(!volume->fat_dirty){
        return FAT_OK;
    }

    // FAT is dirty, write FAT to all copies
    for(uint8_t i = 0; i < volume->num_fats; i++){
        
        // calculate start sector for current FAT copy
        uint32_t fat_sector = volume->fat_begin_sector + 
                                (i * volume->fat_size_sectors);
        
        // write FAT cache to current copy
        int result = volume->device->write_sectors(volume->device->device_data,
                                                   fat_sector,
                                                   volume->fat_size_sectors,
                                                   volume->fat_cache);
    
        if(result != 0){
            return FAT_ERR_DEVICE_ERROR;
        }
    }

    volume->fat_dirty = false;
    return FAT_OK;
}

fat_error_t fat_unmount(fat_volume_t *volume){

    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    // flush pending changes
    fat_error_t err = fat_flush(volume);
    if(err != FAT_OK){
        // flush failed, continue and return err below
    }

    // free FAT cache memory
    if(!volume->fat_cache){
        free(volume->fat_cache);
        volume->fat_cache = NULL;
    }

    // clear volume structure
    memset(volume, 0, sizeof(fat_volume_t));

    return err;
}