#include "fat_validation.h"
#include "fat_cluster.h"
#include "fat_table.h"
#include <string.h>
#include <stdlib.h>

fat_error_t fat_propagate_device_error(int device_error){

    // parameter validation
    if(device_error == 0){
        return FAT_OK;
    }

    switch(device_error){
        case -1:
            return FAT_ERR_DEVICE_ERROR;
        case -2:
            return FAT_ERR_DEVICE_ERROR;
        case -3:
            return FAT_ERR_READ_ONLY;
        default:
            return FAT_ERR_DEVICE_ERROR;
    }
}

bool fat_is_valid_cluster_number(fat_volume_t *volume, cluster_t cluster){

    // parameter validation
    if(!volume){
        return false;
    }

    if(cluster < 2){
        return false;
    }

    if(cluster >= volume->total_clusters + 2){
        return false;
    }

    return true;
}

uint32_t fat_get_max_file_size(fat_volume_t *volume){
    
    // parameter validation
    if(!volume){
        return 0;
    }

    switch(volume->type){
        case FAT_TYPE_FAT12:
            return FAT12_MAX_FILE_SIZE;
        case FAT_TYPE_FAT16:
            return FAT16_MAX_FILE_SIZE;
        case FAT_TYPE_FAT32:
            return FAT32_MAX_FILE_SIZE;
        default:
            return 0;
    }
}

fat_error_t fat_validate_cluster_range(fat_volume_t *volume, cluster_t cluster){

    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!fat_is_valid_cluster_number(volume, cluster)){
        return FAT_ERR_INVALID_CLUSTER;
    }

    return FAT_OK;
}

fat_error_t fat_validate_file_size_limits(fat_volume_t *volume, 
                                          uint32_t file_size){

    // parameter validation
    if(!volume) {
        return FAT_ERR_INVALID_PARAM;
    }

    uint32_t max_size = fat_get_max_file_size(volume);
    if(file_size > max_size){
        return FAT_ERR_FILE_TOO_LARGE;
    }

    uint32_t max_clusters = volume->total_clusters;
    uint64_t max_cluster_size=(uint64_t)max_clusters * volume->bytes_per_cluster;

    if(file_size > max_cluster_size){
        return FAT_ERR_FILE_TOO_LARGE;
    }

    return FAT_OK;
}

fat_error_t fat_validate_cluster_chain(fat_volume_t *volume, 
                                       cluster_t start_cluster){

    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    fat_error_t err = fat_validate_cluster_range(volume, start_cluster);
    if(err != FAT_OK){
        return err;
    }

    cluster_t slow = start_cluster;
    cluster_t fast = start_cluster;
    uint32_t chain_length = 0;

    while(1){
        err = fat_get_next_cluster(volume, slow, &slow);
        if(err != FAT_OK){
            return err;
        }

        if(fat_is_eoc(volume, slow)){
            break;
        }

        err = fat_validate_cluster_range(volume, slow);
        if(err != FAT_OK){
            return err;
        }

        err = fat_get_next_cluster(volume, fast, &fast);
        if(err != FAT_OK){
            return err;
        }

        if(fat_is_eoc(volume, fast)){
            break;
        }

        err = fat_validate_cluster_range(volume, fast);
        if(err != FAT_OK){
            return err;
        }

        err = fat_get_next_cluster(volume, fast, &fast);
        if(err != FAT_OK){
            return err;
        }

        if(fat_is_eoc(volume, fast)){
            break;
        }

        err = fat_validate_cluster_range(volume, fast);
        if(err != FAT_OK){
            return err;
        }

        if(slow == fast){
            return FAT_ERR_CORRUPTED;
        }

        chain_length++;
        if(chain_length > MAX_CLUSTER_CHAIN_LENGTH){
            return FAT_ERR_CORRUPTED;
        }
    }
    return FAT_OK;
}

fat_error_t fat_check_fat_consistency(fat_volume_t *volume){

    // parameter validation
    if(!volume || volume->num_fats < 2){
        return FAT_ERR_INVALID_PARAM;
    }

    uint8_t *fat1_buffer = malloc(volume->bytes_per_sector);
    uint8_t *fat2_buffer = malloc(volume->bytes_per_sector);

    if(!fat1_buffer || !fat2_buffer){
        free(fat1_buffer);
        free(fat2_buffer);
        return FAT_ERR_NO_MEMORY;
    }

    fat_error_t result = FAT_OK;
    uint32_t fat_start_sector = volume->fat_begin_sector;

    for(uint32_t fat_num=1; fat_num < volume->num_fats; fat_num++){
        uint32_t fat1_sector = fat_start_sector;
        uint32_t fat2_sector = fat_start_sector + 
                              (fat_num * volume->fat_size_sectors);
        for(uint32_t sector=0; sector<volume->fat_size_sectors; sector++){
            int result1=volume->device->read_sectors(volume->device->device_data,
                                                     fat1_sector + sector, 
                                                     1,
                                                     fat1_buffer);
            if(result1 != 0){
                result = fat_propagate_device_error(result1);
                goto cleanup;
            }

            int result2=volume->device->read_sectors(volume->device->device_data,
                                                     fat2_sector + sector, 
                                                     1,
                                                     fat2_buffer);
            if(result2 != 0){
                result = fat_propagate_device_error(result2);
                goto cleanup;
            }
        }
    }
cleanup:
    free(fat1_buffer);
    free(fat2_buffer);
    return result;
}

fat_error_t fat_check_volume_integrity(fat_volume_t *volume){

    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    fat_error_t err;

    err = fat_check_fat_consistency(volume);
    if(err != FAT_OK){
        return err;
    }

    // validate root cluser
    if(volume->type == FAT_TYPE_FAT32){
        err = fat_validate_cluster_range(volume, volume->root_cluster);
        if(err != FAT_OK){
            return err;
        }

        err = fat_validate_cluster_chain(volume, volume->root_cluster);
        if(err != FAT_OK){
            return err;
        }
    }

    /* TODO additional integrity checks
    - FS info sector validation
    - sample directory entry validation
    - free cluster count validation
    */

    return FAT_OK;
}

fat_error_t fat_validate_api_parameters_mount(fat_block_device_t *device, 
                                              fat_volume_t *volume){

    // parameter validation
    if(!device){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!device->read_sectors || !device->write_sectors){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    return FAT_OK;
}

fat_error_t fat_validate_api_parameters_open(fat_volume_t *volume, 
                                             const char *path, 
                                             int flags, 
                                             fat_file_t **file){
    
    // parameter validation
    if(!volume){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!path || strlen(path) == 0){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!file){
        return FAT_ERR_INVALID_PARAM;
    }

    int access_mode = flags & (FAT_O_RDONLY | FAT_O_WRONLY | FAT_O_RDWR);
    if(access_mode == 0){
        return FAT_ERR_INVALID_PARAM;
    }

    if((flags & FAT_O_RDONLY) && (flags & FAT_O_WRONLY)){
        return FAT_ERR_INVALID_PARAM;
    }
    
    return FAT_OK;
}

fat_error_t fat_validate_api_parameters_read(fat_file_t *file, 
                                             void *buffer, 
                                             size_t size){

    // parameter validation
    if(!file){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!buffer && size > 0){
        return FAT_ERR_INVALID_PARAM;
    }

    if(size == 0){
        return FAT_OK;
    }

    if(!(file->flags & (FAT_O_RDONLY | FAT_O_RDWR))){
        return FAT_ERR_INVALID_PARAM;
    }

    return FAT_OK;
}

fat_error_t fat_validate_api_parameters_write(fat_file_t *file, 
                                              const void *buffer, 
                                              size_t size){

    // parameter validation
    if(!file){
        return FAT_ERR_INVALID_PARAM;
    }

    if(!buffer && size > 0){
        return FAT_ERR_INVALID_PARAM;
    }

    if(size == 0){
        // 0 size write
        return FAT_OK;
    }

    if(!(file->flags & (FAT_O_WRONLY | FAT_O_RDWR))){
        return FAT_ERR_INVALID_PARAM;
    }

    uint32_t new_size = file->position + size;
    return fat_validate_file_size_limits(file->volume, new_size);
}