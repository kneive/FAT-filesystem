#include "unity.h"
#include "fat_boot.h"
#include "fat_block_device.h"
#include "fat_types.h"
#include <string.h>

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

void create_fat16_boot_sector(uint8_t *buffer, uint32_t target_clusters){
    create_minimal_boot_sector(buffer);

    uint8_t sectors_per_cluster = 4;
    uint32_t root_dir_sectors = 32;
    uint32_t fat_size = 256;
    uint32_t reserved = 1;
    uint32_t num_fats = 2;

    uint32_t first_data_sector = reserved + (num_fats * fat_size) + root_dir_sectors;
    uint32_t data_sectors = target_clusters * sectors_per_cluster;
    uint32_t total_sectors = first_data_sector + data_sectors;

    buffer[13] = sectors_per_cluster;

    buffer[22] = fat_size & 0xFF;
    buffer[23] = (fat_size >> 8) & 0xFF;

    buffer[32] = total_sectors & 0xFF;
    buffer[33] = (total_sectors >> 8) & 0xFF;
    buffer[34] = (total_sectors >> 16) & 0xFF;
    buffer[35] = (total_sectors >> 24) & 0xFF;
}

void create_fat32_boot_sector(uint8_t *buffer, uint32_t target_clusters){
    create_minimal_boot_sector(buffer);

    //root_entry_count
    buffer[17] = 0x00;
    buffer[18] = 0x00;
    //fat_size_16
    buffer[22] = 0x00;
    buffer[23] = 0x00;

    uint8_t sectors_per_cluster = 8;
    uint32_t fat_size_32 = 2048;
    uint32_t reserved = 32;
    uint32_t num_fats = 2;

    uint32_t first_data_sector = reserved + (num_fats * fat_size_32);
    uint32_t data_sectors = target_clusters * sectors_per_cluster;
    uint32_t total_sectors = first_data_sector + data_sectors;

    //sectors_per_cluster
    buffer[13] = sectors_per_cluster;
    //reserved_sector_count
    buffer[14] = reserved & 0xFF;
    buffer[15] = (reserved >> 8) & 0xFF;
    //total_sectors_32
    buffer[32] = total_sectors & 0xFF;
    buffer[33] = (total_sectors >> 8) & 0xFF;
    buffer[34] = (total_sectors >> 16) & 0xFF;
    buffer[35] = (total_sectors >> 24) & 0xFF;
    //extended BPB
    //fat_size_32
    buffer[36] = fat_size_32 & 0xFF;
    buffer[37] = (fat_size_32 >> 8) & 0xFF;
    buffer[38] = (fat_size_32 >> 16) & 0xFF;
    buffer[39] = (fat_size_32 >> 24) & 0xFF;
    //root_cluster
    buffer[44] = 0x02;
    buffer[45] = 0x00;
    buffer[46] = 0x00;
    buffer[47] = 0x00;
}

void write_boot_sector_to_device(fat_block_device_t *device, uint8_t *buffer){
    device->write_sectors(device->device_data, 0, 1, buffer);
}

void setUp(void){ /* pass */ }

void tearDown(void) { /* pass */ }

void test_parse_boot_sector_valid_fat16(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 10000);
    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);
    
    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT16(512, boot_sector.bytes_per_sector);
    TEST_ASSERT_EQUAL_UINT8(2, boot_sector.num_fats);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_valid_fat32(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_fat32_boot_sector(buffer, 100000);
    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL_UINT16(512, boot_sector.bytes_per_sector);
    TEST_ASSERT_EQUAL_UINT32(0, boot_sector.root_entry_count);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_without_device(void){
    
    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(NULL, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_parse_boot_sector_without_boot_sector(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    fat_error_t err = fat_parse_boot_sector(device, NULL);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_invalid_signature(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);
    
    buffer[510] = 0x00;
    buffer[511] = 0x00;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_bytes_per_sector_small(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[11] = 0x00;
    buffer[12] = 0x01;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_bytes_per_sector_no_power_of_2(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[11] = 0xE5;
    buffer[12] = 0x03;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_sectors_per_cluster_zero(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[13] = 0x00;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_sectors_per_cluster_no_power_of_2(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[13] = 0x03;
    
    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_num_fats_zero(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[16] = 0x00;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_parse_boot_sector_reserved_sector_count_zero(void){

    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t buffer[512];
    create_minimal_boot_sector(buffer);

    buffer[14] = 0x00;
    buffer[15] = 0x00;

    write_boot_sector_to_device(device, buffer);

    fat_boot_sector_t boot_sector;
    fat_error_t err = fat_parse_boot_sector(device, &boot_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_BOOT_SECTOR, err);

    fat_block_device_destroy(device);
}

void test_type_fat12(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 4000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT12, type);
}

void test_type_fat12_boundary(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 4084);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT12, type);
}

void test_type_fat16(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 10000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT16, type);
}

void test_type_fat16_boundary_low(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 4085);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT16, type);
}

void test_type_fat16_boundary_up(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 65524);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT16, type);
}

void test_type_fat32(void){

    uint8_t buffer[512];
    create_fat32_boot_sector(buffer, 100000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT32, type);
}

void test_type_fat32_boundary(void){

    uint8_t buffer[512];
    create_fat32_boot_sector(buffer, 65525);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_type_t type;
    fat_error_t err = fat_determine_type(&boot_sector, &type);

    TEST_ASSERT_EQUAL(FAT_OK, err);
    TEST_ASSERT_EQUAL(FAT_TYPE_FAT32, type);
}

void test_type_without_boot_sector(void){

    fat_type_t type;
    fat_error_t err = fat_determine_type(NULL, &type);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_type_without_type(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 10000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_error_t err = fat_determine_type(&boot_sector, NULL);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_fat16_calculate_data_region(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 10000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    uint32_t first_data_sector;
    fat_error_t err = fat_calculate_data_region(&boot_sector, &first_data_sector);

    TEST_ASSERT_EQUAL(FAT_OK, err);

    //reserved + (num_fats * fat_size) + root_dir_sectors
    uint32_t expected = 1 + (2 * 256) + 32;

    TEST_ASSERT_EQUAL_UINT32(expected, first_data_sector);
}

void test_fat32_calculate_data_region(void){

    uint8_t buffer[512];
    create_fat32_boot_sector(buffer, 100000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    uint32_t first_data_sector;
    fat_error_t err = fat_calculate_data_region(&boot_sector, &first_data_sector);
    
    TEST_ASSERT_EQUAL(FAT_OK, err);

    //reserved + (num_fats * fat_size_32) + root_dir_sectors
    uint32_t expected = 32 + (2 * 2048) + 0;

    TEST_ASSERT_EQUAL_UINT32(expected, first_data_sector);
}

void test_calculate_data_region_without_boot_sector(void){

    uint32_t first_data_sector;
    fat_error_t err = fat_calculate_data_region(NULL, &first_data_sector);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_calculate_data_region_without_first_data_sector(void){

    uint8_t buffer[512];
    create_fat16_boot_sector(buffer, 10000);

    fat_boot_sector_t boot_sector;
    memcpy(&boot_sector, buffer, sizeof(fat_boot_sector_t));

    fat_error_t err = fat_calculate_data_region(&boot_sector, NULL);

    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

int main(void){
    UNITY_BEGIN();

    RUN_TEST(test_parse_boot_sector_valid_fat16);
    RUN_TEST(test_parse_boot_sector_valid_fat32);
    RUN_TEST(test_parse_boot_sector_without_device);
    RUN_TEST(test_parse_boot_sector_without_boot_sector);
    RUN_TEST(test_parse_boot_sector_invalid_signature);
    RUN_TEST(test_parse_boot_sector_bytes_per_sector_small);
    RUN_TEST(test_parse_boot_sector_bytes_per_sector_no_power_of_2);
    RUN_TEST(test_parse_boot_sector_sectors_per_cluster_zero);
    RUN_TEST(test_parse_boot_sector_sectors_per_cluster_no_power_of_2);
    RUN_TEST(test_parse_boot_sector_num_fats_zero);
    RUN_TEST(test_parse_boot_sector_reserved_sector_count_zero);

    RUN_TEST(test_type_fat12);
    RUN_TEST(test_type_fat12_boundary);
    RUN_TEST(test_type_fat16);
    RUN_TEST(test_type_fat16_boundary_low);
    RUN_TEST(test_type_fat16_boundary_up);
    RUN_TEST(test_type_fat32);
    RUN_TEST(test_type_fat32_boundary);
    RUN_TEST(test_type_without_boot_sector);
    RUN_TEST(test_type_without_type);

    RUN_TEST(test_fat16_calculate_data_region);
    RUN_TEST(test_fat32_calculate_data_region);
    RUN_TEST(test_calculate_data_region_without_boot_sector);
    RUN_TEST(test_calculate_data_region_without_first_data_sector);

    return UNITY_END();
}