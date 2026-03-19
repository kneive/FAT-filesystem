#include "unity.h"
#include "fat_validation.h"
#include "fat_types.h"
#include "fat_volume.h"
#include "fat_file.h"
#include "fat_block_device.h"
#include <string.h>

fat_volume_t* create_volume(fat_type_t type, uint32_t total_clusters){
    fat_volume_t *volume = malloc(sizeof(fat_volume_t));
    memset(volume, 0, sizeof(fat_volume_t));

    volume->type = type;
    volume->total_clusters = total_clusters;
    volume->bytes_per_sector = 512;
    volume->sectors_per_cluster = 4;
    volume->bytes_per_cluster = 512 * 4;
    volume->num_fats = 2;
    volume->fat_size_sectors = 256;
    volume->fat_begin_sector = 1;

    return volume;
}

fat_file_t* create_file(fat_volume_t *volume, int flags){
    fat_file_t *file = malloc(sizeof(fat_file_t));
    memset(file, 0, sizeof(fat_file_t));

    file->volume = volume;
    file->flags = flags;
    file->position = 0;

    return file;
}

void setUp(){ /* pass */ }

void tearDown(){ /* pass */ }

void test_propagate_device_error_success(void){
    fat_error_t err = fat_propagate_device_error(0);
    TEST_ASSERT_EQUAL(FAT_OK, err);
}

void test_propagate_device_error_generic(void){
    fat_error_t err = fat_propagate_device_error(-1);
    TEST_ASSERT_EQUAL(FAT_ERR_DEVICE_ERROR, err);
}

void test_propagate_device_error_read_only(void){
    fat_error_t err = fat_propagate_device_error(-3);
    TEST_ASSERT_EQUAL(FAT_ERR_READ_ONLY, err);
}

void test_propagate_device_error_unknown(void){
    fat_error_t err = fat_propagate_device_error(-42);
    TEST_ASSERT_EQUAL(FAT_ERR_DEVICE_ERROR, err);
}

// test cluster number

void test_is_valid_cluster_number_volume_NULL(void){
    bool result = fat_is_valid_cluster_number(NULL, 10);
    TEST_ASSERT_FALSE(result);
}

void test_is_valid_cluster_number_zero(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    bool result = fat_is_valid_cluster_number(volume, 0);
    TEST_ASSERT_FALSE(result);
    
    free(volume);
}

void test_is_valid_cluster_number_first_cluster(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    bool result = fat_is_valid_cluster_number(volume, 1);
    TEST_ASSERT_FALSE(result);

    free(volume);
}

void test_is_valid_cluster_number_first_valid_cluster(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    bool result = fat_is_valid_cluster_number(volume, 2);
    TEST_ASSERT_TRUE(result);
    
    free(volume);
}

void test_is_valid_cluster_number_random(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    bool result = fat_is_valid_cluster_number(volume, 42);
    TEST_ASSERT_TRUE(result);

    free(volume);
}

void test_is_valid_cluster_number_oob(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    
    bool result = fat_is_valid_cluster_number(volume, 1002);
    TEST_ASSERT_FALSE(result);

    free(volume);
}

// max filsize

void test_get_max_file_size_volume_NULL(void){
    uint32_t max_size = fat_get_max_file_size(NULL);
    TEST_ASSERT_EQUAL_UINT32(0, max_size);
}

void test_get_max_file_size_fat12(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT12, 1000);

    uint32_t max_size = fat_get_max_file_size(volume);
    TEST_ASSERT_EQUAL_UINT32(FAT12_MAX_FILE_SIZE, max_size);

    free(volume);
}

void test_get_max_file_size_fat16(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    uint32_t max_size = fat_get_max_file_size(volume);
    TEST_ASSERT_EQUAL_UINT32(FAT16_MAX_FILE_SIZE, max_size);

    free(volume);
}

void test_get_max_file_size_fat32(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT32, 1000);

    uint32_t max_size = fat_get_max_file_size(volume);
    TEST_ASSERT_EQUAL_UINT32(FAT32_MAX_FILE_SIZE, max_size);

    free(volume);
}

void test_get_max_file_size_invalid_fat_type(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    volume->type = 99;

    uint32_t max_size = fat_get_max_file_size(volume);
    TEST_ASSERT_EQUAL_UINT32(0, max_size);

    free(volume);
}

// cluster range

void test_validate_cluster_range_volume_NULL(void){
    fat_error_t err = fat_validate_cluster_range(NULL, 10);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_cluster_range_invalid_cluster(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    
    fat_error_t err = fat_validate_cluster_range(volume, 1);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_CLUSTER, err);

    free(volume);
}

void test_validate_cluster_range_valid_cluster(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    fat_error_t err = fat_validate_cluster_range(volume, 42);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(volume);
}

void test_validate_cluster_range_oob(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    fat_error_t err = fat_validate_cluster_range(volume, 5000);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_CLUSTER, err);

    free(volume);
}

// file size limits

void test_validate_file_size_limits_volume_NULL(void){
    fat_error_t err = fat_validate_file_size_limits(NULL, 1000);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_file_size_limits_within_limit(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);

    fat_error_t err = fat_validate_file_size_limits(volume, 1024*1024);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(volume);
}

void test_validate_file_size_limits_exceed_limits(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT12, 1000);
    
    fat_error_t err = fat_validate_file_size_limits(volume, 20*1024*1024);
    TEST_ASSERT_EQUAL(FAT_ERR_FILE_TOO_LARGE, err);

    free(volume);
}

void test_validate_file_size_limits_exceed_cluster_limit(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT32, 100);

    fat_error_t err = fat_validate_file_size_limits(volume, 300000);
    TEST_ASSERT_EQUAL(FAT_ERR_FILE_TOO_LARGE, err);

    free(volume);
}

void test_validate_file_size_limits_boundary(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 10000);

    fat_error_t err = fat_validate_file_size_limits(volume, 20480000);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(volume);
}

// api parameters mount

void test_validate_api_parameters_mount_device_NULL(void){
    fat_volume_t volume;
    fat_error_t err = fat_validate_api_parameters_mount(NULL, &volume);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_api_parameters_mount_volume_NULL(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    fat_error_t err = fat_validate_api_parameters_mount(device, NULL);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    fat_block_device_destroy(device);
}

void test_validate_api_parameters_mount_read_sectors_NULL(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);
    fat_volume_t volume;

    device->read_sectors = NULL;

    fat_error_t err = fat_validate_api_parameters_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    fat_block_device_destroy(device);
}

void test_validate_api_parameters_mount_write_sectors_NULL(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);
    fat_volume_t volume;

    device->write_sectors = NULL;

    fat_error_t err = fat_validate_api_parameters_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    fat_block_device_destroy(device);
}

void test_validate_api_parameters_mount_valid(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);
    fat_volume_t volume;

    fat_error_t err = fat_validate_api_parameters_mount(device, &volume);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    fat_block_device_destroy(device);
}

// validate api parameters open

void test_validate_api_parameters_open_volume_NULL(void){
    fat_file_t *file;
    fat_error_t err = fat_validate_api_parameters_open(NULL, "/test.txt", 
                                                        FAT_O_RDONLY, &file);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_api_parameters_open_path_NULL(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file;

    fat_error_t err = fat_validate_api_parameters_open(volume, NULL, 
                                                        FAT_O_RDONLY, &file);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
    free(volume);
}

void test_validate_api_parameters_open_path_EMPTY(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file;

    fat_error_t err = fat_validate_api_parameters_open(volume, "", 
                                                        FAT_O_RDONLY, &file);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(volume);
}

void test_validate_api_parameters_open_file_NULL(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    
    fat_error_t err = fat_validate_api_parameters_open(volume, "/test.txt", 
                                                        FAT_O_RDONLY, NULL);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(volume);
}

void test_validate_api_parameters_open_no_access_mode(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file;

    // 0 = no access mode set
    fat_error_t err = fat_validate_api_parameters_open(volume, "/test.txt", 
                                                        0, &file);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(volume);
}


void test_validate_api_parameters_open_conflicting_flags(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file;

    fat_error_t err = fat_validate_api_parameters_open(volume, "/test.txt", 
                                            FAT_O_RDONLY | FAT_O_WRONLY, &file);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(volume);
}

void test_validate_api_parameters_open_valid(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file;

    fat_error_t err = fat_validate_api_parameters_open(volume, "/test.txt", 
                                                        FAT_O_RDONLY, &file);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(volume);
}

// validate api parameters read

void test_validate_api_parameters_read_file_NULL(void){
    uint8_t buffer[100];
    fat_error_t err = fat_validate_api_parameters_read(NULL, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_api_parameters_read_buffer_NULL(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_RDONLY);

    fat_error_t err = fat_validate_api_parameters_read(file, NULL, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_read_zero_size(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_RDONLY);

    fat_error_t err = fat_validate_api_parameters_read(file, NULL, 0);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_read_WRONLY(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_WRONLY);
    uint8_t buffer[100];

    fat_error_t err = fat_validate_api_parameters_read(file, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_read_valid(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_RDONLY);
    uint8_t buffer[100];

    fat_error_t err = fat_validate_api_parameters_read(file, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(file);
    free(volume);
}

// validate api parameters write

void test_validate_api_parameters_write_file_NULL(void){
    uint8_t buffer[100];
    memset(buffer, 0x00, 100);
    fat_error_t err = fat_validate_api_parameters_write(NULL, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);
}

void test_validate_api_parameters_write_buffer_NULL(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_WRONLY);

    fat_error_t err = fat_validate_api_parameters_write(file, NULL, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_write_zero_size(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_WRONLY);

    fat_error_t err = fat_validate_api_parameters_write(file, NULL, 0);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_write_RDONLY(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_RDONLY);
    uint8_t buffer[100];
    memset(buffer, 0xCD, 100);
    
    fat_error_t err = fat_validate_api_parameters_write(file, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_ERR_INVALID_PARAM, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_write_exceed_size(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_WRONLY);
    uint8_t buffer[100];
    memset(buffer, 0xCD, 100);
    
    file->position = 0;
    fat_error_t err = fat_validate_api_parameters_write(file, buffer, 20*1024*1024);
    TEST_ASSERT_EQUAL(FAT_ERR_FILE_TOO_LARGE, err);

    free(file);
    free(volume);
}

void test_validate_api_parameters_write_valid(void){
    fat_volume_t *volume = create_volume(FAT_TYPE_FAT16, 1000);
    fat_file_t *file = create_file(volume, FAT_O_WRONLY);
    uint8_t buffer[100];
    memset(buffer, 0xCD, 100);

    fat_error_t err = fat_validate_api_parameters_write(file, buffer, 100);
    TEST_ASSERT_EQUAL(FAT_OK, err);

    free(file);
    free(volume);
}

int main(void){
    UNITY_BEGIN();

    RUN_TEST(test_propagate_device_error_success);
    RUN_TEST(test_propagate_device_error_generic);
    RUN_TEST(test_propagate_device_error_read_only);
    RUN_TEST(test_propagate_device_error_unknown);

    RUN_TEST(test_is_valid_cluster_number_volume_NULL);
    RUN_TEST(test_is_valid_cluster_number_zero);
    RUN_TEST(test_is_valid_cluster_number_first_cluster);
    RUN_TEST(test_is_valid_cluster_number_first_valid_cluster);
    RUN_TEST(test_is_valid_cluster_number_random);
    RUN_TEST(test_is_valid_cluster_number_oob);

    RUN_TEST(test_get_max_file_size_volume_NULL);
    RUN_TEST(test_get_max_file_size_fat12);
    RUN_TEST(test_get_max_file_size_fat16);
    RUN_TEST(test_get_max_file_size_fat32);
    RUN_TEST(test_get_max_file_size_invalid_fat_type);

    RUN_TEST(test_validate_cluster_range_volume_NULL);
    RUN_TEST(test_validate_cluster_range_invalid_cluster);
    RUN_TEST(test_validate_cluster_range_valid_cluster);
    RUN_TEST(test_validate_cluster_range_oob);

    RUN_TEST(test_validate_file_size_limits_volume_NULL);
    RUN_TEST(test_validate_file_size_limits_within_limit);
    RUN_TEST(test_validate_file_size_limits_exceed_limits);
    RUN_TEST(test_validate_file_size_limits_exceed_cluster_limit);
    RUN_TEST(test_validate_file_size_limits_boundary);

    RUN_TEST(test_validate_api_parameters_mount_device_NULL);
    RUN_TEST(test_validate_api_parameters_mount_volume_NULL);
    RUN_TEST(test_validate_api_parameters_mount_read_sectors_NULL);
    RUN_TEST(test_validate_api_parameters_mount_write_sectors_NULL);
    RUN_TEST(test_validate_api_parameters_mount_valid);

    RUN_TEST(test_validate_api_parameters_open_volume_NULL);
    RUN_TEST(test_validate_api_parameters_open_path_NULL);
    RUN_TEST(test_validate_api_parameters_open_path_EMPTY);
    RUN_TEST(test_validate_api_parameters_open_file_NULL);
    RUN_TEST(test_validate_api_parameters_open_no_access_mode);
    RUN_TEST(test_validate_api_parameters_open_conflicting_flags);
    RUN_TEST(test_validate_api_parameters_open_valid);

    RUN_TEST(test_validate_api_parameters_read_file_NULL);
    RUN_TEST(test_validate_api_parameters_read_buffer_NULL);
    RUN_TEST(test_validate_api_parameters_read_zero_size);
    RUN_TEST(test_validate_api_parameters_read_WRONLY);
    RUN_TEST(test_validate_api_parameters_read_valid);

    RUN_TEST(test_validate_api_parameters_write_file_NULL);
    RUN_TEST(test_validate_api_parameters_write_buffer_NULL);
    RUN_TEST(test_validate_api_parameters_write_zero_size);
    RUN_TEST(test_validate_api_parameters_write_RDONLY);
    RUN_TEST(test_validate_api_parameters_write_exceed_size);
    RUN_TEST(test_validate_api_parameters_write_valid);
}