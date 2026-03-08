#include "fat_volume.h"
#include "fat_block_device.h"
#include "fat_dir_list.h"
#include <stdio.h>
#include <string.h>

#define DEFAULT_IMAGE "../resources/fat32.img"


int main(){

    const char *filename = DEFAULT_IMAGE;

    
    printf("This is a dummy driver mounting 'fat32.img'\n\n");


    fat_block_device_t *device = fat_block_device_file_create(filename, 0);

    if(!device) {
        fprintf(stderr, "Failed to open disk image\n");
        return 1;
    }

    fat_volume_t volume;
    fat_error_t err = fat_mount(device, &volume);
    if (err != FAT_OK){
        fprintf(stderr, "Failed to mount volume: %d\n", err);
        return 1;
    }

    fat_dir_t *dir;
    err = fat_opendir(&volume, "/", &dir);
    if(err != FAT_OK){
        fprintf(stderr, "Failed to open root directory: %d\n", err);
        fat_unmount(&volume);
        return 1;
    }

    printf("Root directory listing:\n\n");

    fat_dir_entry_info_t info;
    while(fat_readdir(dir, &info) == FAT_OK){
        if (strcmp(info.long_name, ".") == 0 || strcmp(info.long_name, "..") == 0){
            continue;
        }
    
        printf("%-30s %10u %s\n",
                info.long_name,
                info.file_size,
                info.is_directory ? "DIR" : "FILE");
    }

    fat_closedir(dir);
    fat_unmount(&volume);
    
    return 0;

}
