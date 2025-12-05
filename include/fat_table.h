#ifndef FAT_TABLE_H
#define FAT_TABLE_H

#include <stdint.h>
#include "fat_types.h"
#include "fat_volume.h"

fat_error_t fat_read_entry(fat_volume_t *volume, 
                           cluster_t cluster, uint32_t *value);

fat_error_t fat_write_entry(fat_volume_t *volume, 
                            cluster_t cluster, 
                            uint32_t value);

#endif