// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include "device_types.h"
#include "global_monitor.h"
#include "action_manager.h"
#include "../plugins/flash_device.h"

// 测试用的回调函数
static void test_callback(void* data) {
    if (data) {
        printf("Test callback executed with value: 0x%X\n", *(uint32_t*)data);
    } else {
        printf("Test callback executed with NULL data!\n");
    }
}

int main() {
    printf("Starting device simulator...\n");
    
    // 初始化各个子系统
    device_manager_t* dm = device_manager_init();
    if (!dm) {
        printf("Failed to init device manager\n");
        return -1;
    }
    
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("Failed to create action manager\n");
        device_manager_destroy(dm);
        return -1;
    }
    
    global_monitor_t* gm = global_monitor_create(am);
    if (!gm) {
        printf("Failed to create global monitor\n");
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 获取FLASH设备操作接口
    device_ops_t* flash_ops = get_flash_device_ops();
    if (!flash_ops) {
        printf("Failed to get FLASH device ops\n");
        goto cleanup;
    }
    
    // 注册FLASH设备类型
    if (device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", flash_ops) != 0) {
        printf("Failed to register FLASH device type\n");
        goto cleanup;
    }
    
    // 创建FLASH设备实例
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 1);
    if (!flash) {
        printf("Failed to create FLASH device\n");
        goto cleanup;
    }
    
    printf("Testing FLASH device...\n");
    
    printf("Testing FLASH device basic operations...\n");
    
    // 测试FLASH设备基本操作
    uint32_t value = 0;
    
    // 写入状态寄存器（使能写操作）
    printf("Setting write enable...\n");
    device_ops_t ops = dm->types[DEVICE_TYPE_FLASH].ops;
    if (ops.write) {
        if (ops.write(flash, FLASH_STATUS_REG, STATUS_WEL) != 0) {
            printf("Failed to enable write\n");
            goto cleanup;
        }
    } else {
        printf("Invalid device operations\n");
        goto cleanup;
    }
    
    // 写入数据
    printf("Writing data 0x55 to address 0x%04X...\n", FLASH_DATA_START);
    if (ops.write && ops.write(flash, FLASH_DATA_START, 0x55) != 0) {
        printf("Failed to write data\n");
        goto cleanup;
    }
    
    // 读取数据
    printf("Reading data from address 0x%04X...\n", FLASH_DATA_START);
    if (ops.read && ops.read(flash, FLASH_DATA_START, &value) == 0) {
        printf("Read data: 0x%02X\n", value);
    } else {
        printf("Failed to read data\n");
        goto cleanup;
    }
    
    printf("FLASH device tests completed successfully!\n");
    
    // 创建测试规则
    action_rule_t* rule = action_rule_create(0x1000, 0xAA, 
        ACTION_TYPE_CALLBACK, 0x2000, 0x55);
    if (!rule) {
        printf("Failed to create action rule\n");
        goto cleanup;
    }
    
    rule->callback = test_callback;
    printf("Adding rule to action manager...\n");
    if (action_manager_add_rule(am, rule) != 0) {
        printf("Failed to add rule to action manager\n");
        free(rule);
        goto cleanup;
    }
    
    // 添加监视点
    printf("Adding watch point...\n");
    if (global_monitor_add_watch(gm, 0x1000, 0xAA, rule) != 0) {
        printf("Failed to add watch point\n");
        free(rule);
        goto cleanup;
    }
    
    free(rule);  // 规则已被复制，可以释放原始内存
    
    // 测试地址变化触发
    printf("Testing address change trigger...\n");
    global_monitor_on_address_change(gm, 0x1000, 0xAA);
    
    printf("All tests completed successfully!\n");
    
cleanup:
    // 清理资源
    printf("Cleaning up resources...\n");
    
    if (gm) {
        printf("Destroying global monitor...\n");
        global_monitor_destroy(gm);
        printf("Global monitor destroyed.\n");
        gm = NULL;
    }
    
    if (am) {
        printf("Destroying action manager...\n");
        action_manager_destroy(am);
        printf("Action manager destroyed.\n");
        am = NULL;
    }
    
    if (dm) {
        printf("Destroying device manager...\n");
        device_manager_destroy(dm);
        printf("Device manager destroyed.\n");
        dm = NULL;
    }
    
    printf("Cleanup completed.\n");
    return 0;
}