#include "unity.h"
#include "fat_block_device.h"
#include <string.h>
#include <stdio.h>

void setUp(void){}

void tearDown(void){
    remove("test_device.img");
}

// Memory device tests

void test_memory_device_create(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);
    
    TEST_ASSERT_NOT_NULL(device);
    
    fat_block_device_destroy(device);
}

void test_memory_device_get_sector_count(void){
    uint32_t expected_sectors = 100;
    fat_block_device_t *device = fat_block_device_memory_create(expected_sectors);
    
    uint32_t actual_sectors = 0;
    int result = device->get_sector_count(device->device_data, &actual_sectors);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT32(expected_sectors, actual_sectors);
    
    fat_block_device_destroy(device);

}

void test_memory_device_get_sector_size(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint32_t sector_size = 0;
    int result = device->get_sector_size(device->device_data, &sector_size);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT32(FAT_SECTOR_SIZE, sector_size);

    fat_block_device_destroy(device);

}

void test_memory_device_write_single_sector(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);
    uint8_t write_buffer[FAT_SECTOR_SIZE];
    memset(write_buffer, 0xAB, FAT_SECTOR_SIZE);

    int result = device->write_sectors(device->device_data, 5, 1, write_buffer);

    TEST_ASSERT_EQUAL_INT(0, result);

    fat_block_device_destroy(device);
}

void test_memory_device_read_after_write(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t write_buffer[FAT_SECTOR_SIZE];
    memset(write_buffer, 0xCD, FAT_SECTOR_SIZE);
    device->write_sectors(device->device_data, 10, 1, write_buffer);

    uint8_t read_buffer[FAT_SECTOR_SIZE];
    memset(read_buffer, 0x00, FAT_SECTOR_SIZE);
    int result = device->read_sectors(device->device_data, 10, 1, read_buffer);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_MEMORY(write_buffer, read_buffer, FAT_SECTOR_SIZE);

    fat_block_device_destroy(device);
}

void test_memory_device_read_multiple_sectors(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t write_buffer[FAT_SECTOR_SIZE * 3];
    memset(write_buffer, 0x11, FAT_SECTOR_SIZE);
    memset(write_buffer + FAT_SECTOR_SIZE, 0x22, FAT_SECTOR_SIZE);
    memset(write_buffer + FAT_SECTOR_SIZE * 2, 0x33, FAT_SECTOR_SIZE);

    device->write_sectors(device->device_data, 20, 3, write_buffer);

    uint8_t read_buffer[FAT_SECTOR_SIZE * 3];
    int result = device->read_sectors(device->device_data, 20, 3, read_buffer);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_MEMORY(write_buffer, read_buffer, FAT_SECTOR_SIZE * 3);

    fat_block_device_destroy(device);
}

void test_memory_device_read_out_of_bounds(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t buffer[FAT_SECTOR_SIZE];

    int result = device->read_sectors(device->device_data, 100, 1, buffer);
    
    TEST_ASSERT_EQUAL_INT(-1, result);  // should fail

    fat_block_device_destroy(device);
}

void test_memory_device_write_out_of_bounds(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t buffer[FAT_SECTOR_SIZE];
    memset(buffer, 0XFF, FAT_SECTOR_SIZE);

    int result = device->write_sectors(device->device_data, 99, 2, buffer);

    TEST_ASSERT_EQUAL_INT(-1, result);

    fat_block_device_destroy(device);
}

void test_memory_device_sector_isolation(void){
    fat_block_device_t *device = fat_block_device_memory_create(100);

    uint8_t pattern1[FAT_SECTOR_SIZE];
    uint8_t pattern2[FAT_SECTOR_SIZE];
    memset(pattern1, 0xAA, FAT_SECTOR_SIZE);
    memset(pattern2, 0xBB, FAT_SECTOR_SIZE);

    device->write_sectors(device->device_data, 50, 1, pattern1);
    device->write_sectors(device->device_data, 51, 1, pattern2);

    uint8_t read1[FAT_SECTOR_SIZE];
    uint8_t read2[FAT_SECTOR_SIZE];
    device->read_sectors(device->device_data, 50, 1, read1);
    device->read_sectors(device->device_data, 51, 1, read2);

    TEST_ASSERT_EQUAL_MEMORY(pattern1, read1, FAT_SECTOR_SIZE);
    TEST_ASSERT_EQUAL_MEMORY(pattern2, read2, FAT_SECTOR_SIZE);

    fat_block_device_destroy(device);
}

// File device tests

void test_file_device_create(void){
    fat_block_device_t *device = fat_block_device_file_create("test_device.img", 100);

    TEST_ASSERT_NOT_NULL(device);

    fat_block_device_destroy(device);
}

void test_file_device_read_after_write(void){
    fat_block_device_t *device = fat_block_device_file_create("test_device.img", 100);

    uint8_t write_buffer[FAT_SECTOR_SIZE];
    memset(write_buffer, 0XEF, FAT_SECTOR_SIZE);
    device->write_sectors(device->device_data, 15, 1, write_buffer);

    uint8_t read_buffer[FAT_SECTOR_SIZE];
    int result = device->read_sectors(device->device_data, 15, 1, read_buffer);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_MEMORY(write_buffer, read_buffer, FAT_SECTOR_SIZE);

    fat_block_device_destroy(device);
}

void test_file_device_persistence(void){
    fat_block_device_t *device = fat_block_device_file_create("test_device.img", 100);

    uint8_t write_buffer[FAT_SECTOR_SIZE];
    memset(write_buffer, 0x42, FAT_SECTOR_SIZE);
    device->write_sectors(device->device_data, 7, 1, write_buffer);

    fat_block_device_destroy(device);

    fat_block_device_t *device2 = fat_block_device_file_create("test_device.img", 100);

    uint8_t read_buffer[FAT_SECTOR_SIZE];
    device2->read_sectors(device2->device_data, 7, 1, read_buffer);

    TEST_ASSERT_EQUAL_MEMORY(write_buffer, read_buffer, FAT_SECTOR_SIZE);

    fat_block_device_destroy(device2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_memory_device_create);
    RUN_TEST(test_memory_device_get_sector_count);
    RUN_TEST(test_memory_device_get_sector_size);
    RUN_TEST(test_memory_device_write_single_sector);
    RUN_TEST(test_memory_device_read_after_write);
    RUN_TEST(test_memory_device_read_multiple_sectors);
    RUN_TEST(test_memory_device_read_out_of_bounds);
    RUN_TEST(test_memory_device_write_out_of_bounds);
    RUN_TEST(test_memory_device_sector_isolation);

    RUN_TEST(test_file_device_create);
    RUN_TEST(test_file_device_read_after_write);
    RUN_TEST(test_file_device_persistence);

    return UNITY_END();
}