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

// 温度报警回调函数
static void temp_alert_callback(void* data) {
    if (data) {
        printf("Temperature alert triggered! Current temperature: %.2f°C\n", 
            (*(uint32_t*)data) * 0.0625);
    }
}

// FPGA中断回调函数
static void fpga_irq_callback(void* data) {
    if (data) {
        printf("FPGA interrupt triggered! IRQ status: 0x%X\n", *(uint32_t*)data);
    }
}

// 定义动作规则表
static const rule_table_entry_t action_rules[] = {
    // 温度传感器报警规则
    {
        .name = "Temperature High Alert",
        .trigger_addr = TEMP_REG,
        .expect_value = 3000,  // 30°C
        .type = ACTION_TYPE_CALLBACK,
        .target_addr = 0,
        .action_value = 0,
        .priority = 1,
        .callback = temp_alert_callback
    },
    // FPGA中断规则
    {
        .name = "FPGA IRQ Handler",
        .trigger_addr = FPGA_IRQ_REG,
        .expect_value = 1,
        .type = ACTION_TYPE_CALLBACK,
        .target_addr = 0,
        .action_value = 0,
        .priority = 2,
        .callback = fpga_irq_callback
    },
    // 测试规则
    {
        .name = "Test Rule",
        .trigger_addr = 0x1000,
        .expect_value = 0xAA,
        .type = ACTION_TYPE_CALLBACK,
        .target_addr = 0x2000,
        .action_value = 0x55,
        .priority = 0,
        .callback = test_callback
    }
};

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
    
    // 添加动作规则
    printf("\nAdding action rules...\n");
    if (action_manager_add_rules_from_table(am, action_rules, 
        sizeof(action_rules) / sizeof(action_rules[0])) != 0) {
        printf("Failed to add action rules\n");
        goto cleanup;
    }
    
    // 为每个规则添加监视点
    printf("Setting up watch points...\n");
    for (size_t i = 0; i < sizeof(action_rules) / sizeof(action_rules[0]); i++) {
        const rule_table_entry_t* entry = &action_rules[i];
        action_rule_t rule = {
            .rule_id = i + 1,  // 从1开始的规则ID
            .trigger_addr = entry->trigger_addr,
            .expect_value = entry->expect_value,
            .type = entry->type,
            .target_addr = entry->target_addr,
            .action_value = entry->action_value,
            .priority = entry->priority,
            .callback = entry->callback
        };
        
        printf("Adding watch point for rule: %s\n", entry->name);
        if (global_monitor_add_watch(gm, entry->trigger_addr, entry->expect_value, &rule) != 0) {
            printf("Failed to add watch point for rule: %s\n", entry->name);
            goto cleanup;
        }
    }
    
    // 测试规则触发
    printf("\nTesting rules...\n");
    
    // 测试温度报警规则
    printf("Testing temperature alert rule...\n");
    global_monitor_on_address_change(gm, TEMP_REG, 3000);
    
    // 测试FPGA中断规则
    printf("Testing FPGA interrupt rule...\n");
    global_monitor_on_address_change(gm, FPGA_IRQ_REG, 1);
    
    // 测试通用规则
    printf("Testing general purpose rule...\n");
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