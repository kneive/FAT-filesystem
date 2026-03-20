#include "unity.h"
#include "fat_volume.h"
#include "fat_boot.h"
#include "fat_block_device.h"
#include "fat_types.h"
#include <string.h>
#include <stdlib.h>

void create_minimal_boot_sector(uint8_t *buffer){
    memset(buffer, 0, 512);

    // boot signature
    buffer[510] = 0x55;
    buffer[511] = 0xAA;

    //bytes_per_sector
    buffer[11] = 0x00;
    buffer[12] = 0x02;
    //sectors_per_cluster
    buffer[13] = 0x01;
    //reserved_sector_count
    buffer[14] = 0x01;
    buffer[15] = 0x00;
    //num_fats
    buffer[16] = 0x02;
    //root_entry_count
    buffer[17] = 0x00;
    buffer[18] = 0x02;
    //total_sectors_16
    buffer[19] = 0x00;
    buffer[20] = 0x00;
    //media_type
    buffer[21] = 0xF8;
    //fat_size_16
    buffer[22] = 0x09;
    buffer[23] = 0x00;
    //sectors_per_track
    buffer[24] = 0x12;
    buffer[25] = 0x00;
    //num_heads
    buffer[26] = 0x02;
    buffer[27] = 0x00;
    //hidden_sectors
    buffer[28] = 0x00;
    buffer[29] = 0x00;
    buffer[30] = 0x00;
    buffer[31] = 0x00;
    //total_sectors_32
    buffer[32] = 0x40;
    buffer[33] = 0x0B;
    buffer[34] = 0x00;
    buffer[35] = 0x00;
}

void create_fat16_boot_sector(uint8_t *buffer, 
                              uint16_t bytes_per_sector,
                              uint8_t sectors_per_cluster,
                              uint16_t reserved_sectors,
                              uint8_t num_fats,
                              uint16_t root_entries,
                              uint16_t fat_size,
                              uint32_t total_sectors) {

    create_minimal_boot_sector(buffer);

    buffer[11] = bytes_per_sector & 0xFF;
    buffer[12] = (bytes_per_sector >> 8) & 0xFF;

    buffer[13] = sectors_per_cluster;

    buffer[14] = reserved_sectors & 0xFF;
    buffer[15] = (reserved_sectors >> 8) & 0xFF;

    buffer[16] = num_fats;

    buffer[17] = root_entries & 0xFF;
    buffer[18] = (root_entries >> 8) & 0xFF;

    buffer[22] = fat_size & 0xFF;
    buffer[23] = (fat_size >> 8) & 0xFF;

    buffer[32] = total_sectors & 0xFF;
    buffer[33] = (total_sectors >> 8) & 0xFF;
    buffer[34] = (total_sectors >> 16) & 0xFF;
    buffer[35] = (total_sectors >> 24) & 0xFF;
}

void create_fat32_boot_sector(uint8_t *buffer,
                              uint16_t bytes_per_sector,
                              uint8_t sectors_per_cluster,
                              uint16_t reserved_sectors,
                              uint8_t num_fats,
                              uint32_t fat_size_32,
                              uint32_t total_sectors,
                              uint32_t root_cluster) {

    create_minimal_boot_sector(buffer);
    buffer[17] = 0x00;
    buffer[18] = 0x00;

    buffer[22] = 0x00;
    buffer[23] = 0x00;

    buffer[11] = bytes_per_sector & 0xFF;
    buffer[12] = (bytes_per_sector >> 8) & 0xFF;

    buffer[13] = sectors_per_cluster;

    buffer[14] = reserved_sectors & 0xFF;
    buffer[15] = (reserved_sectors >> 8) & 0xFF;

    buffer[16] = num_fats;

    buffer[32] = total_sectors & 0xFF;
    buffer[33] = (total_sectors >> 8) & 0xFF;
    buffer[34] = (total_sectors >> 16) & 0xFF;
    buffer[35] = (total_sectors >> 24) & 0xFF;

    buffer[36] = fat_size_32 & 0xFF;
    buffer[37] = (fat_size_32 >> 8) & 0xFF;
    buffer[38] = (fat_size_32 >> 16) & 0xFF;
    buffer[39] = (fat_size_32 >> 24) & 0xFF;

    buffer[44] = root_cluster & 0xFF;
    buffer[45] = (root_cluster >> 8) & 0xFF;
    buffer[46] = (root_cluster >> 16) & 0xFF;
    buffer[47] = (root_cluster >> 24) & 0xFF;

}

fat_block_device_t* create_fat16_device(void){
    fat_block_device_t *device = fat_block_device_memory_create(10000);

    uint8_t boot_sector[512];
    create_fat16_boot_sector(boot_sector, 512, 4, 1, 2, 512, 256, 20000);
    device->write_sectors(device->device_data, 0, 1, boot_sector);

    uint8_t fat_buffer[512];
    memset(fat_buffer, 0, 512);

    // first FAT copy
    for(uint32_t i= 0; i < 256; i++){
        device->write_sectors(device->device_data, 1 + i, 1, fat_buffer);
    }

    // second FAT copy
    for(uint32_t i = 0; i < 256; i++){
        device->write_sectors(device->device_data, 257 + i, 1, fat_buffer);
    }

    return device;
}

fat_block_device_t* create_fat32_device(void){
    fat_block_device_t *device = fat_block_device_memory_create(100000);

    uint8_t boot_sector[512];
    create_fat32_boot_sector(boot_sector, 512, 8, 32, 2, 2048, 800000, 2);
    device->write_sectors(device->device_data, 0, 1, boot_sector);

    uint8_t fat_buffer[512];
    memset(fat_buffer, 0, 512);

    // first FAT copy
    for(uint32_t i = 0; i < 2048; i++){
        device->write_sectors(device->device_data, 32 + i, 1, fat_buffer);
    }

    // second FAT copy
    for(uint32_t i = 0; i < 2048; i++){
        device->write_sectors(device->device_data, 2080 + i, 1, fat_buffer);
    }

    return device;
}


fat_block_device_t* create_device_with_invalid_boot_sector(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t boot_sector[512];
    memset(boot_sector, 0, 512);
    // invalid signature
    boot_sector[510] = 0x00;
    boot_sector[511] = 0x00;

    device->write_sectors(device->device_data, 0, 1, boot_sector);
    
    return device;
}

void setUp(void) { /* pass */ }

void tearDown(void) { /* pass */ }


// mount

void test_mount_valid_fat16_volume(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT16, volume.type);
    TEST_ASSERT_NOT_NULL(volume.fat_cache);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_valid_fat32_volume(void){
    fat_block_device_t *device = create_fat32_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT32, volume.type);
    TEST_ASSERT_NOT_NULL(volume.fat_cache);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_device_NULL(void){
    fat_volume_t volume;

    fat_error_t err = fat_mount(NULL, &volume);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_mount_volume_NULL(void){
    fat_block_device_t *device = create_fat16_device();

    fat_error_t err = fat_mount(device, NULL);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    fat_block_device_destroy(device);
}

void test_mount_invalid_boot_sector(void){
    fat_block_device_t *device = create_device_with_invalid_boot_sector();
    fat_volume_t volume;
    
    fat_error_t err = fat_mount(device, &volume);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_mount_boot_sector_parameters(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    TEST_ASSERT_EQUAL_UINT16(512, volume.bytes_per_sector);
    TEST_ASSERT_EQUAL_UINT8(4, volume.sectors_per_cluster);
    TEST_ASSERT_EQUAL_UINT32(2048, volume.bytes_per_cluster);
    TEST_ASSERT_EQUAL_UINT16(1, volume.reserved_sector_count);
    TEST_ASSERT_EQUAL_UINT8(2, volume.num_fats);
    TEST_ASSERT_EQUAL_UINT16(512, volume.root_entry_count);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_offset(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(1, volume.fat_begin_sector);
    TEST_ASSERT_EQUAL_UINT32(32, volume.root_dir_sectors);
    TEST_ASSERT_EQUAL_UINT32(545, volume.data_begin_sector);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_total_clusters(void){

    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(4863, volume.total_clusters);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_fat16_fat_size_sectors(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(256, volume.fat_size_sectors);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_fat32_fat_size_sectors(void){
    fat_block_device_t *device = create_fat32_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(2048, volume.fat_size_sectors);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_fat_cache(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_NOT_NULL(volume.fat_cache);

    uint32_t expected_size = 256 * 512;
    TEST_ASSERT_EQUAL_UINT32(expected_size, volume.fat_cache_size);
    TEST_ASSERT_FALSE(volume.fat_dirty);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_fat32_root_cluster(void){
    fat_block_device_t *device = create_fat32_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(2, volume.root_cluster);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_fat16_root_cluster_ZERO(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT32(0, volume.root_cluster);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_mount_device_pointers(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_error_t err = fat_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_PTR(device, volume.device);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

// flush

void test_flush_volume_NULL(void){
    fat_error_t err = fat_flush(NULL);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_flush_fat_clean_no_write(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    TEST_ASSERT_FALSE(volume.fat_dirty);

    fat_error_t err = fat_flush(&volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_flush_dirty_fat_writes_all_copies(void){

    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    volume.fat_dirty = true;

    memset(volume.fat_cache, 0xAB, volume.fat_cache_size);

    fat_error_t err = fat_flush(&volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_FALSE(volume.fat_dirty);

    uint8_t read_buffer[512];
    device->read_sectors(device->device_data, 1, 1, read_buffer);
    TEST_ASSERT_EQUAL_UINT8(0xAB, read_buffer[0]);

    device->read_sectors(device->device_data, 257, 1, read_buffer);
    TEST_ASSERT_EQUAL_UINT8(0xAB, read_buffer[0]);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_flush_clear_dirty_flag(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    volume.fat_dirty = true;

    fat_error_t err = fat_flush(&volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_FALSE(volume.fat_dirty);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

void test_flush_write_to_sectors(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    volume.fat_dirty = true;
    memset(volume.fat_cache, 0xCD, 512);

    fat_flush(&volume);

    uint8_t buffer1[512];
    device->read_sectors(device->device_data, 1, 1, buffer1);
    TEST_ASSERT_EQUAL_UINT8(0xCD, buffer1[0]);

    uint8_t buffer2[512];
    device->read_sectors(device->device_data, 257, 1, buffer2);
    TEST_ASSERT_EQUAL_UINT8(0xCD, buffer2[0]);

    fat_unmount(&volume);
    fat_block_device_destroy(device);
}

// unmount

void test_unmount_volume_NULL(void){
    fat_error_t err = fat_unmount(NULL);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_unmount_clean_volume(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    fat_error_t err = fat_unmount(&volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    fat_block_device_destroy(device);
}

void test_unmount_dirty_volume_flushes(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);

    volume.fat_dirty = true;
    memset(volume.fat_cache, 0xEF, 512);

    fat_error_t err = fat_unmount(&volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    uint8_t buffer[512];
    device->read_sectors(device->device_data, 1, 1, buffer);
    TEST_ASSERT_EQUAL_UINT8(0xEF, buffer[0]);

    fat_block_device_destroy(device);
}

void test_unmount_free_fat_cache(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    TEST_ASSERT_NOT_NULL(volume.fat_cache);
    fat_unmount(&volume);
    TEST_ASSERT_NULL(volume.fat_cache);

    fat_block_device_destroy(device);
}

void test_unmount_clear_volume_structure(void){
    fat_block_device_t *device = create_fat16_device();
    fat_volume_t volume;

    fat_mount(device, &volume);
    TEST_ASSERT_NOT_EQUAL(0, volume.bytes_per_cluster);
    TEST_ASSERT_NOT_NULL(volume.device);

    fat_unmount(&volume);
    TEST_ASSERT_EQUAL(0, volume.bytes_per_sector);
    TEST_ASSERT_NULL(volume.device);
    TEST_ASSERT_NULL(volume.fat_cache);

    fat_block_device_destroy(device);
}

int main(void){

    UNITY_BEGIN();

    RUN_TEST(test_mount_valid_fat16_volume);
    RUN_TEST(test_mount_valid_fat32_volume);
    RUN_TEST(test_mount_device_NULL);
    RUN_TEST(test_mount_volume_NULL);
    RUN_TEST(test_mount_invalid_boot_sector);
    RUN_TEST(test_mount_boot_sector_parameters);
    RUN_TEST(test_mount_offset);
    RUN_TEST(test_mount_total_clusters);
    RUN_TEST(test_mount_fat16_fat_size_sectors);
    RUN_TEST(test_mount_fat32_fat_size_sectors);
    RUN_TEST(test_mount_fat_cache);
    RUN_TEST(test_mount_fat32_root_cluster);
    RUN_TEST(test_mount_fat16_root_cluster_ZERO);
    RUN_TEST(test_mount_device_pointers);

    RUN_TEST(test_flush_volume_NULL);
    RUN_TEST(test_flush_fat_clean_no_write);
    RUN_TEST(test_flush_dirty_fat_writes_all_copies);
    RUN_TEST(test_flush_clear_dirty_flag);
    RUN_TEST(test_flush_write_to_sectors);

    RUN_TEST(test_unmount_volume_NULL);
    RUN_TEST(test_unmount_clean_volume);
    RUN_TEST(test_unmount_dirty_volume_flushes);
    RUN_TEST(test_unmount_free_fat_cache);
    RUN_TEST(test_unmount_clear_volume_structure);

    return UNITY_END();
}