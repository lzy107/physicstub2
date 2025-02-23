// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "device_types.h"
#include "global_monitor.h"
#include "action_manager.h"
#include "device_registry.h"
#include "../plugins/flash_device.h"
#include "../plugins/temp_sensor.h"
#include "../plugins/fpga_device.h"

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
    
    // 初始化设备注册表
    if (device_registry_init(dm) != 0) {
        printf("Failed to initialize device registry\n");
        goto cleanup;
    }
    
    // 创建FLASH设备实例
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 1);
    if (!flash) {
        printf("Failed to create FLASH device\n");
        goto cleanup;
    }
    
    // 创建温度传感器实例
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 1);
    if (!temp_sensor) {
        printf("Failed to create temperature sensor\n");
        goto cleanup;
    }
    
    printf("Testing devices...\n");
    
    // 测试FLASH设备
    printf("\nTesting FLASH device basic operations...\n");
    uint32_t value = 0;
    
    // 写入状态寄存器（使能写操作）
    printf("Setting write enable...\n");
    device_ops_t flash_ops = dm->types[DEVICE_TYPE_FLASH].ops;
    if (flash_ops.write) {
        if (flash_ops.write(flash, FLASH_STATUS_REG, STATUS_WEL) != 0) {
            printf("Failed to enable write\n");
            goto cleanup;
        }
    } else {
        printf("Invalid device operations\n");
        goto cleanup;
    }
    
    // 写入数据
    printf("Writing data 0x55 to address 0x%04X...\n", FLASH_DATA_START);
    if (flash_ops.write && flash_ops.write(flash, FLASH_DATA_START, 0x55) != 0) {
        printf("Failed to write data\n");
        goto cleanup;
    }
    
    // 读取数据
    printf("Reading data from address 0x%04X...\n", FLASH_DATA_START);
    if (flash_ops.read && flash_ops.read(flash, FLASH_DATA_START, &value) == 0) {
        printf("Read data: 0x%02X\n", value);
    } else {
        printf("Failed to read data\n");
        goto cleanup;
    }
    
    printf("FLASH device tests completed successfully!\n");
    
    // 测试温度传感器
    printf("\nTesting temperature sensor...\n");
    device_ops_t temp_ops = dm->types[DEVICE_TYPE_TEMP_SENSOR].ops;
    
    // 读取当前温度
    if (temp_ops.read && temp_ops.read(temp_sensor, TEMP_REG, &value) == 0) {
        printf("Current temperature: %.4f°C\n", value * 0.0625);
    } else {
        printf("Failed to read temperature\n");
        goto cleanup;
    }
    
    // 配置温度报警
    printf("Configuring temperature alerts...\n");
    if (temp_ops.write && temp_ops.write(temp_sensor, CONFIG_REG, CONFIG_ALERT) != 0) {
        printf("Failed to configure alerts\n");
        goto cleanup;
    }
    
    // 设置温度阈值
    if (temp_ops.write && temp_ops.write(temp_sensor, TLOW_REG, 2000) != 0) {  // 20°C
        printf("Failed to set low temperature threshold\n");
        goto cleanup;
    }
    
    if (temp_ops.write && temp_ops.write(temp_sensor, THIGH_REG, 2800) != 0) {  // 28°C
        printf("Failed to set high temperature threshold\n");
        goto cleanup;
    }
    
    printf("Temperature sensor configured successfully!\n");
    
    // 监控温度变化
    printf("\nMonitoring temperature for 3 seconds...\n");
    for (int i = 0; i < 3; i++) {
        if (temp_ops.read && temp_ops.read(temp_sensor, TEMP_REG, &value) == 0) {
            printf("Temperature: %.4f°C\n", value * 0.0625);
        }
        sleep(1);
    }
    
    printf("Temperature sensor tests completed successfully!\n");
    
    // 测试FPGA设备
    printf("\nTesting FPGA device...\n");
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 1);
    if (!fpga) {
        printf("Failed to create FPGA device\n");
        goto cleanup;
    }
    
    device_ops_t fpga_ops = dm->types[DEVICE_TYPE_FPGA].ops;
    
    // 使能FPGA
    printf("Enabling FPGA...\n");
    if (fpga_ops.write && fpga_ops.write(fpga, FPGA_CONFIG_REG, CONFIG_ENABLE | CONFIG_IRQ_EN) != 0) {
        printf("Failed to enable FPGA\n");
        goto cleanup;
    }
    
    // 读取状态寄存器
    printf("Reading FPGA status...\n");
    if (fpga_ops.read && fpga_ops.read(fpga, FPGA_STATUS_REG, &value) == 0) {
        printf("FPGA status: 0x%08X\n", value);
    } else {
        printf("Failed to read FPGA status\n");
        goto cleanup;
    }
    
    // 写入数据
    printf("Writing data to FPGA memory...\n");
    for (int i = 0; i < 10; i++) {
        if (fpga_ops.write && fpga_ops.write(fpga, FPGA_DATA_START + i, i + 1) != 0) {
            printf("Failed to write data\n");
            goto cleanup;
        }
    }
    
    // 读取数据
    printf("Reading back data from FPGA memory...\n");
    for (int i = 0; i < 10; i++) {
        if (fpga_ops.read && fpga_ops.read(fpga, FPGA_DATA_START + i, &value) == 0) {
            printf("Data[%d] = 0x%02X\n", i, value);
        } else {
            printf("Failed to read data\n");
            goto cleanup;
        }
    }
    
    // 启动FPGA操作
    printf("Starting FPGA operation...\n");
    if (fpga_ops.write && fpga_ops.write(fpga, FPGA_CONTROL_REG, CTRL_START) != 0) {
        printf("Failed to start FPGA operation\n");
        goto cleanup;
    }
    
    // 等待操作完成
    printf("Waiting for operation to complete...\n");
    int timeout = 10;  // 最多等待1秒
    do {
        if (fpga_ops.read && fpga_ops.read(fpga, FPGA_STATUS_REG, &value) != 0) {
            printf("Failed to read status\n");
            goto cleanup;
        }
        if (value & STATUS_DONE) break;
        usleep(100000);  // 等待100ms
    } while (--timeout > 0);
    
    if (timeout > 0) {
        printf("FPGA operation completed successfully\n");
    } else {
        printf("FPGA operation timed out\n");
    }
    
    // 检查中断状态
    if (fpga_ops.read && fpga_ops.read(fpga, FPGA_IRQ_REG, &value) == 0) {
        printf("IRQ status: 0x%08X\n", value);
    }
    
    printf("FPGA device tests completed successfully!\n");
    
    // 创建测试规则
    action_rule_t* rule = action_rule_create(0x1000, 0xAA, 
        ACTION_TYPE_CALLBACK, 0x2000, 0x55);
    if (!rule) {
        printf("Failed to create action rule\n");
        goto cleanup;
    }
    
    rule->callback = test_callback;
    printf("\nAdding rule to action manager...\n");
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
    printf("\nCleaning up resources...\n");
    
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