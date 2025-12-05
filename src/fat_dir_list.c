#include "fat_dir_list.h"
#include "fat_path.h"
#include "fat_cluster.h"
#include "fat_lfn.h"
#include "fat_root.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void fat_convert_short_name(const uint8_t *short_name, char *output){
    
    // parameter validation
    if(!short_name || !output){
        if(output) 
            output[0] = '\0';
        return;
    }

    char name_part[9] = {0};
    char ext_part[4] = {0};

    // extract name part
    int name_len = 8;
    while(name_len > 0 && short_name[name_len-1] == ' '){
        name_len--;
    }

    if(name_len > 0){
            memcpy(name_part, short_name, name_len);
            name_part[name_len] = '\0';
    }

    // extract extension part
    int ext_len = 3;
    while(ext_len > 0 && short_name[8 + ext_len - 1] == ' '){
        ext_len--;
    }

    if(ext_len > 0){
        memcpy(ext_part, &short_name[8], ext_len);
        ext_part[ext_len] = '\0';
    }

    // build output string
    if(name_len > 0 && ext_len > 0){
        snprintf(output, 13, "%s.%s", name_part, ext_part);
    } else if(name_len > 0){
        strcpy(output, name_part);
    } else if(ext_len > 0){
        snprintf(output, 13, ".%s", ext_part);
    } else {
        output[0] = '\0';
    }
}

void fat_extract_entry_info(const fat_dir_entry_t *entry, const char *long_name, 
                            fat_dir_entry_info_t *info){
                            
    // parameter validation
    if(!entry || !info){
        return;
    }

    memset(info, 0, sizeof(fat_dir_entry_info_t));

    fat_convert_short_name(entry->name, info->short_name);

    if(long_name && strlen(long_name) > 0){
        strncpy(info->long_name, long_name, sizeof(info->long_name) -1);
        info->long_name[sizeof(info->long_name)-1] = '\0';
    } else {
        strcpy(info->long_name, info->short_name);
    }

    info->attributes = entry->attr;
    info->is_directory = (entry->attr & FAT_ATTR_DIRECTORY) != 0;
    info->is_hidden = (entry->attr & FAT_ATTR_HIDDEN) != 0;
    info->is_readonly = (entry->attr & FAT_ATTR_READ_ONLY) != 0;

    info->file_size = entry->file_size;
    info->start_cluster = ((uint32_t)entry->first_cluster_high << 16) |
                            entry->first_cluster_low;
    
    info->create_date = entry->create_date;
    info->create_time = entry->create_time;
    info->modify_date = entry->write_date;
    info->modify_time = entry->write_time;
    info->access_date = entry->access_date;
}

fat_error_t fat_load_directory_cluster(fat_dir_t *dir, cluster_t cluster){

    // parameter validation
    if(!dir || !dir->volume){
        return FAT_ERR_INVALID_PARAM;
    }

    uint32_t sector;
    uint32_t sectors_to_read;

    if(dir->is_root_fat12){
        // FAT12/16 root
        uint32_t entries_per_sector = dir->volume->bytes_per_sector / 32;
        uint32_t root_start = dir->volume->reserved_sector_count +
                              (dir->volume->num_fats * dir->volume->fat_size_sectors);

        uint32_t sector_index = dir->current_entry_index / entries_per_sector;
        sector = root_start + sector_index;
        sectors_to_read = 1;
        if(dir->current_entry_index >= dir->max_entries){
            return FAT_ERR_EOF;
        }
    } else {
        // FAT32 root / subdirectory
        if(cluster < 2 || fat_is_eoc(dir->volume, cluster)){
            return FAT_ERR_EOF;
        }

        sector = fat_cluster_to_sector(dir->volume, cluster);
        sectors_to_read = dir->volume->sectors_per_cluster;
    }

    int result = dir->volume->device->read_sectors(dir->volume->device->device_data,
                                                   sector, sectors_to_read,
                                                   dir->cluster_buffer);
    if(result != 0){
        return FAT_ERR_DEVICE_ERROR;
    }

    dir->current_cluster = cluster;
    dir->cluster_offset = 0;

    return FAT_OK;
}

fat_error_t fat_opendir(fat_volume_t *volume, const char *path, fat_dir_t **dir){

    // parameter validation
    if(!volume || !path || !dir){
        return FAT_ERR_INVALID_PARAM;
    }

    *dir = NULL;

    fat_dir_entry_t dir_entry;
    cluster_t parent_cluster;
    uint32_t entry_index;

    fat_error_t err = fat_resolve_path(volume, path, &dir_entry, &parent_cluster, 
                                       &entry_index);
    if(err != FAT_OK){
        return err;
    }

    if(!(dir_entry.attr & FAT_ATTR_DIRECTORY)){
        return FAT_ERR_NOT_A_DIRECTORY;
    }

    fat_dir_t *new_dir = malloc(sizeof(fat_dir_t));
    if(!new_dir){
        return FAT_ERR_NO_MEMORY;
    }

    memset(new_dir, 0, sizeof(fat_dir_t));
    new_dir->volume = volume;
    new_dir->dir_cluster = fat_get_entry_cluster(volume, &dir_entry);
    new_dir->current_cluster = new_dir->dir_cluster;
    new_dir->current_entry_index = 0;
    new_dir->cluster_offset = 0;
    
    new_dir->is_root_fat12 = (new_dir->dir_cluster == 0 &&
                              volume->type != FAT_TYPE_FAT32);
    
    if(new_dir->is_root_fat12){
        // FAT12/16 root - allocate buffer for 1 sector
        new_dir->max_entries = volume->root_entry_count;
        new_dir->cluster_buffer = malloc(volume->bytes_per_sector);
    } else {
        // allocate buffer for 1 cluster
        new_dir->max_entries = 0;
        new_dir->cluster_buffer = malloc(volume->bytes_per_cluster);
    }

    if(!new_dir->cluster_buffer){
        free(new_dir);
        return FAT_ERR_NO_MEMORY;
    }

    err = fat_load_directory_cluster(new_dir, new_dir->current_cluster);
    if(err != FAT_OK){
        free(new_dir->cluster_buffer);
        free(new_dir);
        return err;
    }

    *dir = new_dir;
    return FAT_OK;
}

fat_error_t fat_readdir(fat_dir_t *dir, fat_dir_entry_info_t *info){

    // parameter validation
    if(!dir || !info){
        return FAT_ERR_INVALID_PARAM;
    }

    uint32_t entries_per_buffer;
    if(dir->is_root_fat12){
        entries_per_buffer = dir->volume->bytes_per_sector / 32;
    } else {
        entries_per_buffer = dir->volume->bytes_per_cluster / 32;
    }

    while(1){
        // check if we need to load the next sector/cluster
        if(dir->cluster_offset >= entries_per_buffer){
            if(dir->is_root_fat12){
                // FAT12/16
                dir->current_entry_index = ((dir->current_entry_index / 
                                            entries_per_buffer) + 1) *
                                            entries_per_buffer;
                
                if(dir->current_entry_index >= dir->max_entries){
                    return FAT_ERR_EOF;
                }

                fat_error_t err = fat_load_directory_cluster(dir, 0);
                if(err != FAT_OK){
                    return err;
                }
            } else {
                // FAT32 / subdirectory
                cluster_t next_cluster;
                fat_error_t err = fat_get_next_cluster(dir->volume, 
                                                       dir->current_cluster,
                                                       &next_cluster);
                if(err != FAT_OK || fat_is_eoc(dir->volume, next_cluster)){
                    return FAT_ERR_EOF;
                }

                err = fat_load_directory_cluster(dir, next_cluster);
                if(err != FAT_OK){
                    return err;
                }

                dir->current_entry_index = ((dir->current_entry_index / 
                                             entries_per_buffer) + 1) * 
                                             entries_per_buffer;                
            }
        }

        // get current directory entry
        fat_dir_entry_t *entry = (fat_dir_entry_t*)&dir->
                                  cluster_buffer[dir->cluster_offset * 32];

        if(entry->name[0] == FAT_DIR_ENTRY_FREE){
            return FAT_ERR_EOF;
        }

        if(entry->name[0] == FAT_DIR_ENTRY_DELETED){
            dir->cluster_offset++;
            dir->current_entry_index++;
            continue;
        }

        if(entry->attr == FAT_ATTR_LONG_NAME){
            dir->cluster_offset++;
            dir->current_entry_index++;
            continue;
        }

        if(entry->attr & FAT_ATTR_VOLUME_ID){
            dir->cluster_offset++;
            dir->current_entry_index++;
            continue;
        }

        // entry is valid - check for LFN
        char long_filename[256] = {0};
        bool has_lfn = false;

        if(dir->current_entry_index > 0){
            uint8_t checksum = fat_calculate_lfn_checksum(entry->name);
            uint32_t lfn_entry_index = dir->current_entry_index;

            fat_error_t lfn_err = fat_read_lfn_sequence(dir->volume, 
                                                        dir->dir_cluster,
                                                        &lfn_entry_index, 
                                                        long_filename,
                                                        sizeof(long_filename), 
                                                        checksum);

            if(lfn_err == FAT_OK && strlen(long_filename) > 0){
                has_lfn = true;
            }
        }

        fat_extract_entry_info(entry, has_lfn ? long_filename : NULL, info);

        dir->cluster_offset++;
        dir->current_entry_index++;

        return FAT_OK;
    }
}

fat_error_t fat_closedir(fat_dir_t *dir){
    
    // parameter validation
    if(!dir){
        return FAT_OK;
    }

    if(dir->cluster_buffer){
        free(dir->cluster_buffer);
    }

    free(dir);

    return FAT_OK;
}