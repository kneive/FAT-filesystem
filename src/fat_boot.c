#include "fat_boot.h"
#include <string.h>

fat_error_t fat_parse_boot_sector(fat_block_device_t *device, 
                                  fat_boot_sector_t *boot_sector){
    
    // validate parameters
    if (!device || !boot_sector){
        return FAT_ERR_INVALID_PARAM;
    }

    uint8_t sector_buffer[512];

    // read boot sector
    int result = device->read_sectors(device->device_data, 0, 1, sector_buffer);
    if(result!=0){
        return FAT_ERR_DEVICE_ERROR;
    }

    // copy to boot sector structure
    memcpy(boot_sector, sector_buffer, sizeof(fat_boot_sector_t));

    // validate boot sector signature (bytes 510-511)
    uint16_t signature = *((uint16_t*)&sector_buffer[510]);
    if(signature != FAT_BOOT_SIGNATURE){
        return FAT_ERR_INVALID_BOOT_SECTOR;
    }

    // validate basic BPB fields
    if(boot_sector->bytes_per_sector < 512 || 
        (boot_sector->bytes_per_sector & 
        (boot_sector->bytes_per_sector - 1))!=0){
            return FAT_ERR_INVALID_BOOT_SECTOR;
    }

    if  (boot_sector->sectors_per_cluster == 0 ||
        (boot_sector->sectors_per_cluster & 
        (boot_sector->sectors_per_cluster - 1))!=0){
            return FAT_ERR_INVALID_BOOT_SECTOR;
        }

    if(boot_sector->num_fats == 0){
        return FAT_ERR_INVALID_BOOT_SECTOR;
    }

    if(boot_sector->reserved_sector_count == 0){
        return FAT_ERR_INVALID_BOOT_SECTOR;
    }

    return FAT_OK;
}

fat_error_t fat_determine_type(const fat_boot_sector_t *boot_sector, 
                               fat_type_t *type){

    // validate parameters
    if(!boot_sector || !type){
        return FAT_ERR_INVALID_PARAM;
    }

    // calculate total sectors
    uint32_t total_sectors;
    if(boot_sector->total_sectors_16 != 0){
        total_sectors = boot_sector->total_sectors_16;
    } else {
        total_sectors = boot_sector->total_sectors_32;
    }

    // calculate root directory sectors
    uint32_t root_dir_sectors = ((boot_sector->root_entry_count * 32) + 
                                (boot_sector->bytes_per_sector - 1)) /
                                boot_sector->bytes_per_sector;

    // calculate FAT size
    uint32_t fat_size;
    if(boot_sector->fat_size_16 != 0){
        fat_size = boot_sector->fat_size_16;
    } else{
        fat_size = boot_sector->extended.fat32.fat_size_32;
    }

    // calculate first data sector
    uint32_t first_data_sector = boot_sector->reserved_sector_count + 
                                    (boot_sector->num_fats * fat_size) + 
                                    root_dir_sectors;

    // calculate data sectors
    uint32_t data_sectors = total_sectors - first_data_sector;

    // calculate total clusters
    uint32_t total_clusters = data_sectors / boot_sector->sectors_per_cluster;

    /* determine FAT type: 
    - FAT12 < 4085 clusters, 
    - FAT16 4086 - 65524 clusters, 
    - FAT32 > 65524 clusters
    */

    if(total_clusters < 4085){
        *type = FAT_TYPE_FAT12;
    } else if (total_clusters < 65525){
        *type = FAT_TYPE_FAT16;
    } else {
        *type = FAT_TYPE_FAT32;
    }

    return FAT_OK;
}

fat_error_t fat_calculate_data_region(const fat_boot_sector_t *boot_sector, 
                                      uint32_t *first_data_sector) {
    
    // validate parameters
    if(!boot_sector || !first_data_sector) {
        return FAT_ERR_INVALID_PARAM;
    }

    // calculate root directory sectors
    uint32_t root_dir_sectors = ((boot_sector->root_entry_count * 32) + 
                                (boot_sector->bytes_per_sector - 1)) / 
                                boot_sector->bytes_per_sector;
    
    // get FAT size
    uint32_t fat_size;
    if(boot_sector->fat_size_16 != 0){
        fat_size = boot_sector->fat_size_16;
    } else {
        fat_size = boot_sector->extended.fat32.fat_size_32;
    }

    // calculate first data sector
    *first_data_sector = boot_sector->reserved_sector_count + 
                            (boot_sector->num_fats * fat_size) + 
                            root_dir_sectors;

    return FAT_OK;
}