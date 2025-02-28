// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "device_registry.h"
#include "device_types.h"
#include "action_manager.h"
#include "global_monitor.h"
#include "device_test.h"
#include "device_memory.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/fpga/fpga_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/common/device_tests.h"
#include "../plugins/common/test_rule_provider.h"

// 示例：创建带配置的Flash设备
device_instance_t* create_configured_flash_device(device_manager_t* dm) {
    // 创建内存区域配置
    memory_region_config_t mem_regions[2];
    
    // 寄存器区域
    mem_regions[0].base_addr = 0x00;
    mem_regions[0].unit_size = 4;  // 4字节单位
    mem_regions[0].length = 8;     // 8个寄存器
    
    // 数据区域
    mem_regions[1].base_addr = FLASH_DATA_START;
    mem_regions[1].unit_size = 4;  // 4字节单位
    mem_regions[1].length = (FLASH_MEM_SIZE - FLASH_DATA_START) / 4;  // 剩余空间
    
    // 创建规则
    device_rule_t rules[1];
    
    // 规则1：当状态寄存器的值为0x01时触发
    rules[0].addr = FLASH_REG_STATUS;
    rules[0].expected_value = 0x01;
    rules[0].expected_mask = 0xFF;
    rules[0].active = 1;
    
    // 因使用全局规则配置，不再需要手动创建目标处理动作
    rules[0].targets = NULL;
    
    // 创建设备配置
    device_config_t config;
    config.mem_regions = mem_regions;
    config.region_count = 2;
    config.rules = rules;
    config.rule_count = 1;
    
    // 创建设备实例
    device_instance_t* flash = device_create_with_config(dm, DEVICE_TYPE_FLASH, 1, &config);
    
    return flash;
}

int main() {
    printf("初始化设备模拟器...\n");
    
    // 创建设备管理器
    device_manager_t* dm = device_manager_init();
    if (!dm) {
        printf("创建设备管理器失败\n");
        return 1;
    }
    printf("设备管理器创建成功\n");
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("创建动作管理器失败\n");
        device_manager_destroy(dm);
        return 1;
    }
    printf("动作管理器创建成功\n");
    
    // 创建全局监视器
    global_monitor_t* gm = global_monitor_create(am, dm);
    if (!gm) {
        printf("创建全局监视器失败\n");
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    printf("全局监视器创建成功\n");
    
    // 注册设备类型
    printf("开始注册设备类型...\n");
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops());
    printf("温度传感器设备类型注册成功\n");
    
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops());
    printf("FPGA设备类型注册成功\n");
    
    register_flash_device_type(dm);
    printf("Flash设备类型注册成功\n");
    
    printf("设备注册表已初始化，包含 %d 种设备类型\n", MAX_DEVICE_TYPES);
    
    // 创建设备实例
    printf("开始创建设备实例...\n");
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    if (temp_sensor) {
        printf("温度传感器设备实例创建成功，ID: %d\n", temp_sensor->dev_id);
    } else {
        printf("温度传感器设备实例创建失败\n");
    }
    
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 0);
    if (fpga) {
        printf("FPGA设备实例创建成功，ID: %d\n", fpga->dev_id);
    } else {
        printf("FPGA设备实例创建失败\n");
    }
    
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);
    if (flash) {
        printf("Flash设备实例创建成功，ID: %d\n", flash->dev_id);
    } else {
        printf("Flash设备实例创建失败\n");
    }
    
    if (!temp_sensor || !fpga || !flash) {
        printf("创建设备实例失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 创建带配置的Flash设备
    printf("开始创建带配置的Flash设备...\n");
    device_instance_t* configured_flash = create_configured_flash_device(dm);
    if (configured_flash) {
        printf("成功创建带配置的Flash设备，ID: %d\n", configured_flash->dev_id);
    } else {
        printf("创建带配置的Flash设备失败\n");
    }
    
    // 加载所有规则
    printf("\n开始加载动作规则...\n");
    int rule_count = action_manager_load_all_rules(am);
    if (rule_count < 0) {
        printf("加载动作规则失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    printf("成功加载 %d 条动作规则\n", rule_count);

    printf("系统初始化完成，设备就绪\n");
    
    // 清理资源
    printf("\n开始清理资源...\n");
    printf("销毁全局监视器...\n");
    global_monitor_destroy(gm);
    printf("销毁动作管理器...\n");
    action_manager_destroy(am);
    printf("销毁设备管理器...\n");
    device_manager_destroy(dm);
    printf("资源清理完成\n");
    
    return 0;
}