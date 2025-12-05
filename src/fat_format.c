#include "fat_format.h"
#include "fat_boot.h"
#include "fat_dir.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

fat_error_t fat_calculate_format_parameters(uint32_t total_sectors, 
                                            uint32_t bytes_per_sector, 
                                            fat_type_t preferred_type, 
                                            uint32_t preferred_cluster_size, 
                                            fat_format_params_t *params){

    // parameter validation
    if(!params || total_sectors == 0 || bytes_per_sector == 0){
        return FAT_ERR_INVALID_PARAM;
    }

    memset(params, 0, sizeof(fat_format_params_t));

    params->bytes_per_sector = bytes_per_sector;
    params->total_sectors = total_sectors;
    params->num_fats = 2;

    // calculate total volume size in bytes
    uint64_t total_bytes = (uint64_t)total_sectors * bytes_per_sector;

    // determine cluster size
    uint32_t cluster_size_bytes;
    if(preferred_cluster_size>0){
        cluster_size_bytes = preferred_cluster_size;
    } else {
        // auto-select cluster size base on volume size
        if(total_bytes <= 32 * 1024 * 1024){
            // <= 32MB - 1 sector
            cluster_size_bytes = 512;
        } else if(total_bytes <= 64 * 1024 * 1024){
            // <= 64MB - 2 sectors
            cluster_size_bytes = 1024;
        } else if(total_bytes <= 128 * 1024 * 1024){
            // <= 128MB - 4 sectors
            cluster_size_bytes = 2048;
        } else if(total_bytes <= 256 * 1024 * 1024){
            // <= 256MB - 8 sectors
            cluster_size_bytes = 4096;
        } else if(total_bytes <= 8LL * 1024 * 1024){
            // <= 8GB - 8 sectors
            cluster_size_bytes = 4096;
        } else if(total_bytes <= 16LL * 1024 * 1024){
            // <= 16 GB - 16 sectors
            cluster_size_bytes = 8192;
        } else if(total_bytes <= 32LL * 1024 * 1024){
            // <= 32GB - 32 sectors
            cluster_size_bytes = 16384;
        } else {
            // 64 sectors
            cluster_size_bytes = 32768;
        }
    }

    params->bytes_per_cluster = cluster_size_bytes;
    params->sectors_per_cluster = cluster_size_bytes / bytes_per_sector;

    // validate cluster size
    if (params->sectors_per_cluster == 0 || 
        params->sectors_per_cluster > 128 ||
        (params->sectors_per_cluster & (params->sectors_per_cluster - 1)) != 0){
        // cluster size must be power of 2 and 
        // sectors per cluster mus be <= 128
        return FAT_ERR_INVALID_PARAM;
    }

    // set reserved sectors
    if(preferred_type == FAT_TYPE_FAT32){
        // FAT32
        params->reserved_sectors = 32;
        params->fs_info_sector = 1;
        params->backup_boot_sector = 6;
    } else {
        // FAT12/16
        params->reserved_sectors = 1;
    }

    // FAT12/16 calculate root directory size
    if(preferred_type != FAT_TYPE_FAT32){
        if(total_bytes <= 32 * 1024 * 1024){
            // FAT12 224 root directory entries
            params->root_entry_count = 224;
        } else {
            // FAT16 512 root directory entries
            params->root_entry_count = 512;
        } 
    } else {
        // FAT32
        params->root_entry_count = 0;
        params->root_cluster = 2;
    }

    // FAT12/16 caluclate root directory sectors
    uint32_t root_dir_sectors = 0;
    if(params->root_entry_count > 0) {
        root_dir_sectors = ((params->root_entry_count * 32) +
                            (bytes_per_sector - 1)) / bytes_per_sector;
    }

    // calculate data sectors
    uint32_t fat_size_sectors = 0;
    uint32_t data_sectors = 0;
    uint32_t total_clusters = 0;

    for(int i=0; i<10; i++){
        data_sectors = total_sectors - params->reserved_sectors -
                      (params->num_fats * fat_size_sectors) - root_dir_sectors;
        total_clusters = data_sectors / params->sectors_per_cluster;
        fat_type_t calculated_type;
        if(total_clusters < 4085){
            calculated_type = FAT_TYPE_FAT12;
        } else if(total_clusters > 65525){
            calculated_type = FAT_TYPE_FAT16;
        } else {
            calculated_type = FAT_TYPE_FAT32;
        }

        // use preferred type if set
        if(preferred_type != FAT_TYPE_FAT12 &&
           preferred_type != FAT_TYPE_FAT16 &&
           preferred_type != FAT_TYPE_FAT32) {
           params->fat_type = calculated_type;
           } else {
            params->fat_type = preferred_type;
           }

        // adjust parameters if fat type changed
        if(params->fat_type == FAT_TYPE_FAT32 && params->reserved_sectors == 1){
            params->reserved_sectors = 32;
            params->fs_info_sector = 1;
            params->backup_boot_sector = 6;
            params->root_entry_count = 0;
            params->root_cluster = 2;
            root_dir_sectors = 0;
            continue;
        }

        // calculate required FAT size
        uint32_t fat_entries = total_clusters + 2;
        uint32_t bytes_per_fat_entry;

        switch(params->fat_type){
            case FAT_TYPE_FAT12:
                bytes_per_fat_entry = 2;
                break;
            case FAT_TYPE_FAT16:
                bytes_per_fat_entry = 2;
                break;
            case FAT_TYPE_FAT32:
                bytes_per_fat_entry = 4;
                break;
            default:
                return FAT_ERR_UNSUPPORTED_FAT_TYPE;
        }

        uint32_t fat_size_bytes = fat_entries * bytes_per_fat_entry;
        uint32_t new_fat_size_sectors = (fat_size_bytes + bytes_per_sector-1) /
                                            bytes_per_sector;
        
        if(new_fat_size_sectors == fat_size_sectors){
            break;
        }

        fat_size_sectors = new_fat_size_sectors;
    }
    
    if(total_clusters < 2){
        // volume to small
        return FAT_ERR_INVALID_PARAM;
    }

    switch(params->fat_type){
        case FAT_TYPE_FAT12:
            if(total_clusters >= 4085){
                // to many clusters for FAT12
                return FAT_ERR_INVALID_PARAM;
            }
            break;
        case FAT_TYPE_FAT16:
            if(total_clusters < 4085){
                // not enough clusters for FAT16
                return FAT_ERR_INVALID_PARAM;
            }
            break;
        case FAT_TYPE_FAT32:
            if(total_clusters < 65525){
                // not enough clusters for FAT32
                return FAT_ERR_INVALID_PARAM;
            }
    }
    params->fat_size_sectors = fat_size_sectors;
    params->data_sectors = data_sectors;
    params->total_clusters = total_clusters;

    return FAT_OK;
}

fat_error_t fat_write_boot_sector(fat_block_device_t *device, 
                                  const fat_format_params_t *params, 
                                  const char *volume_label){

    // parameter validation
    if(!device || !params){
        return FAT_ERR_INVALID_PARAM;
    }

    // asllocate sector buffer
    uint8_t *boot_sector = calloc(1, params->bytes_per_sector);
    if(!boot_sector){
        return FAT_ERR_NO_MEMORY;
    }

    fat_boot_sector_t *bs = (fat_boot_sector_t*)boot_sector;

    // set jump instruction
    bs->jmp_boot[0] = 0xEB;
    bs->jmp_boot[1] = 0x3C;
    bs->jmp_boot[2] = 0x90;

    memcpy(bs->oem_name, "FATDRV  ", 8);

    // set BPB parameters
    bs->bytes_per_sector = params->bytes_per_sector;
    bs->sectors_per_cluster = params->sectors_per_cluster;
    bs->reserved_sector_count = params->reserved_sectors;
    bs->num_fats = params->num_fats;
    bs->root_entry_count = params->root_entry_count;

    // set total sectors
    if(params->total_sectors < 65536){
        // FAT12/16
        bs->total_sectors_16 = params->total_sectors;
        bs->total_sectors_32 = 0;
    } else {
        bs->total_sectors_16 = 0;
        bs->total_sectors_32 = params->total_sectors;
    }

    bs->media_type = 0xF8; // fixed disk
    bs->sectors_per_track = 63;
    bs->num_heads = 255;
    bs->hidden_sectors = 0;

    if(params->fat_type == FAT_TYPE_FAT32){
        // FAT32
        bs->fat_size_16 = 0;
        bs->extended.fat32.fat_size_32 = params->fat_size_sectors;
        bs->extended.fat32.ext_flags = 0;
        bs->extended.fat32.fs_version = 0;
        bs->extended.fat32.root_cluster = params->root_cluster;
        bs->extended.fat32.fs_info = params->fs_info_sector;
        bs->extended.fat32.backup_boot_sector = params->backup_boot_sector;
        memset(bs->extended.fat32.reserved, 0, 12);
        bs->extended.fat32.drive_number = 0x80;
        bs->extended.fat32.reserved1 = 0;
        bs->extended.fat32.boot_signature = 0x29;
        bs->extended.fat32.volume_id = (uint32_t)time(NULL);

        if(volume_label && strlen(volume_label) > 0){
            memset(bs->extended.fat32.volume_label, ' ', 11);
            size_t label_len = strlen(volume_label);
            if(label_len>11) 
                label_len = 11;
            memcpy(bs->extended.fat32.volume_label, volume_label, label_len);
        } else {
            memcpy(bs->extended.fat32.volume_label, "NO NAME    ",  11);
        }
        memcpy(bs->extended.fat32.fs_type, "FAT32   ", 8);
    } else {
        // FAT12/16
        bs->fat_size_16 = params->fat_size_sectors;
        bs->extended.fat16.drive_number = 0x80;
        bs->extended.fat16.reserved1 = 0;
        bs->extended.fat16.boot_signature = 0x29;
        bs->extended.fat16.volume_id = (uint32_t)time(NULL);

        if(volume_label && strlen(volume_label) > 0){
            memset(bs->extended.fat16.volume_label, ' ', 11);
            size_t label_len = strlen(volume_label);
            if(label_len > 11)
                label_len = 11;
            memcpy(bs->extended.fat16.volume_label, volume_label, label_len);
        } else {
            memcpy(bs->extended.fat16.volume_label, "NO NAME    ", 11);
        }
        if(params->fat_type == FAT_TYPE_FAT12){
            memcpy(bs->extended.fat16.fs_type, "FAT12   ", 8);
        } else {
            memcpy(bs->extended.fat16.fs_type, "FAT16   ", 8);
        }
    }

    // set boot signature
    boot_sector[params->bytes_per_sector - 2] = 0x55;
    boot_sector[params->bytes_per_sector - 1] = 0xAA;

    int result = device->write_sectors(device->device_data, 0, 1, boot_sector);

    // write backup boot sector
    if(result == 0 && params->fat_type == FAT_TYPE_FAT32 
                   && params->backup_boot_sector > 0 ){
        result = device->write_sectors(device->device_data, 
                                       params->backup_boot_sector, 
                                       1, 
                                       boot_sector);
    }

    free(boot_sector);
    return (result == 0) ? FAT_OK : FAT_ERR_DEVICE_ERROR;
}

fat_error_t fat_initialize_fat_tables(fat_block_device_t *device, 
                                      const fat_format_params_t *params){

    // parameter validation
    if(!device || !params){
        return FAT_ERR_INVALID_PARAM;
    }

    uint8_t *fat_buffer = calloc(1, params->bytes_per_sector);
    if(!fat_buffer){
        return FAT_ERR_NO_MEMORY;
    }

    switch(params->fat_type){
        case FAT_TYPE_FAT12:
            uint8_t *fat12 = fat_buffer;
            fat12[0] = 0xF8;
            fat12[1] = 0xFF;
            fat12[2] = 0xFF;
            break;
        case FAT_TYPE_FAT16:
            uint16_t *fat16 = (uint16_t*)fat_buffer;
            fat16[0] = 0xFFF8;
            fat16[1] = 0xFFFF;
            break;
        case FAT_TYPE_FAT32:
            uint32_t *fat32 = (uint32_t*)fat_buffer;
            fat32[0] = 0x0FFFFFF8;
            fat32[1] = 0x0FFFFFFF;
            fat32[2] = 0x0FFFFFFF;
            break;
        default:
            free(fat_buffer);
            return FAT_ERR_UNSUPPORTED_FAT_TYPE;
    }

    uint32_t fat_start_sector = params->reserved_sectors;
     // write first sector of each FAT copy
     for(uint32_t fat_num = 0; fat_num<params->num_fats; fat_num++){
        
        uint32_t fat_sector = fat_start_sector + 
                                (fat_num * params->fat_size_sectors);
        
        int result = device->write_sectors(device->device_data, 
                                           fat_sector, 
                                           1, 
                                           fat_buffer);
        if(result!=0){
            free(fat_buffer);
            return FAT_ERR_DEVICE_ERROR;
        }
     }

     memset(fat_buffer, 0, params->bytes_per_sector);

     for(uint32_t fat_num=0; fat_num<params->num_fats; fat_num++){
        
        uint32_t fat_sector = fat_start_sector + 
                                (fat_num * params->fat_size_sectors);
        
        for(uint32_t sector=1; sector<params->fat_size_sectors; sector++){
            
            int result = device->write_sectors(device->device_data, 
                                               fat_sector + sector, 
                                               1, 
                                               fat_buffer);
            if(result != 0){
                free(fat_buffer);
                return FAT_ERR_DEVICE_ERROR;
            }
        }
     }
     free(fat_buffer);
     return FAT_OK;
}

fat_error_t fat_create_fs_info_sector(fat_block_device_t *device, 
                                      const fat_format_params_t *params){

    //parameter validation
    if(!device || !params || params->fat_type != FAT_TYPE_FAT32){
        return FAT_ERR_INVALID_PARAM;
    }

    uint8_t *fs_info = calloc(1, params->bytes_per_sector);
    if(!fs_info){
        return FAT_ERR_NO_MEMORY;
    }

    // set FS Info signartures and data
    *(uint32_t*)&fs_info[0] = 0x41615252;
    *(uint32_t*)&fs_info[484] = 0x61417272;
    *(uint32_t*)&fs_info[488] = params->total_clusters - 1;
    *(uint32_t*)&fs_info[492] = 3;
    *(uint32_t*)&fs_info[508] = 0xAA550000;

    int result = device->write_sectors(device->device_data, 
                                       params->fs_info_sector, 
                                       1, 
                                       fs_info);
    
    free(fs_info);

    return (result==0) ? FAT_OK : FAT_ERR_DEVICE_ERROR;
}

fat_error_t fat_initialize_root_directory(fat_block_device_t *device, 
                                          const fat_format_params_t *params, 
                                          const char *volume_label){

    // parameter validation
    if(!device || !params){
        return FAT_ERR_INVALID_PARAM;
    }

    uint8_t *dir_buffer = calloc(1, params->bytes_per_sector);
    if(!dir_buffer){
        return FAT_ERR_NO_MEMORY;
    }

    if(volume_label && strlen(volume_label) > 0){
        fat_dir_entry_t *vol_entry = (fat_dir_entry_t*)dir_buffer;
        
        memset(vol_entry->name, ' ', 11);
        size_t label_len = strlen(volume_label);
        if(label_len > 11)
            label_len = 11;
        memcpy(vol_entry->name, volume_label, label_len);

        vol_entry->attr = FAT_ATTR_VOLUME_ID;
        vol_entry->nt_reserved = 0;
        vol_entry->create_time_tenth = 0;

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if(tm_info){
            uint16_t fat_time = ((tm_info->tm_hour & 0x1F) << 11) |
                                ((tm_info->tm_min & 0x3F) << 5) |
                                ((tm_info->tm_sec / 2) & 0x1F);

            uint16_t fat_date = (((tm_info->tm_year - 80) & 0x7F) << 9) |
                                (((tm_info->tm_mon + 1) & 0x0F) << 5) |
                                (tm_info->tm_mday & 0x1F);

            vol_entry->create_time = fat_time;
            vol_entry->create_date = fat_date;
            vol_entry->write_time = fat_time;
            vol_entry->write_date = fat_date;
            vol_entry->access_date = fat_date;
        }

        vol_entry->first_cluster_high = 0;
        vol_entry->first_cluster_low = 0;
        vol_entry->file_size = 0;
    }

    if(params->fat_type == FAT_TYPE_FAT32){
        // FAT32 init root directory cluster
        uint32_t root_sector = params->reserved_sectors +
                              (params->num_fats * params->fat_size_sectors) +
                              ((params->root_cluster-2) * params->sectors_per_cluster);
        
        for(uint32_t i=0; i<params->sectors_per_cluster; i++){
        
            int result = device->write_sectors(device->device_data, 
                                               root_sector + i, 1, dir_buffer);
        
            if(result!=0){
                free(dir_buffer);
                return FAT_ERR_DEVICE_ERROR;
            }

            if(i==0){
                memset(dir_buffer, 0, params->bytes_per_sector);
            }
        }
    } else {
        // FAT12/16 init root
        uint32_t root_start_sector = params->reserved_sectors +
                                    (params->num_fats * params->fat_size_sectors);

        uint32_t root_sectors = ((params->root_entry_count * 32) +
                                 (params->bytes_per_sector - 1)) / 
                                  params->bytes_per_sector;
        for(uint32_t i=0; i<root_sectors; i++){

            int result = device->write_sectors(device->device_data, 
                                               root_start_sector + i, 
                                               1, 
                                               dir_buffer);

            if(result!=0){
                free(dir_buffer);
                return FAT_ERR_DEVICE_ERROR;
            }

            if(i==0){
                memset(dir_buffer, 0, params->bytes_per_sector);
            }
        }
    }

    free(dir_buffer);
    return FAT_OK;
}

fat_error_t fat_format(fat_block_device_t *device, 
                       fat_type_t fat_type, 
                       uint32_t cluster_size, 
                       const char *volume_label){

    // parameter validation
    if(!device){
        return FAT_ERR_INVALID_PARAM;
    }

    // get device size
    uint32_t total_sectors = 0;
    uint32_t bytes_per_sector = 512;

    // TODO: implement block device interface for device size

    if(total_sectors  == 0){
        return FAT_ERR_INVALID_PARAM;
    }

    // calculate filesystem parameters
    fat_format_params_t params;
    fat_error_t err = fat_calculate_format_parameters(total_sectors, 
                                                      bytes_per_sector, 
                                                      fat_type, 
                                                      cluster_size, 
                                                      &params);
    if(err != FAT_OK){
        return err;
    }

    err = fat_write_boot_sector(device, &params, volume_label);
    if(err != FAT_OK){
        return err;
    }

    err = fat_initialize_fat_tables(device, &params);
    if(err != FAT_OK){
        return err;
    }

    if(params.fat_type == FAT_TYPE_FAT32){
        err = fat_create_fs_info_sector(device, &params);
        if(err != FAT_OK){
            return err;
        }
    }

    err = fat_initialize_root_directory(device, &params, volume_label);
    if(err != FAT_OK){
        return err;
    }

    return FAT_OK;
}