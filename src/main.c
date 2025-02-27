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
    
    // 创建目标处理动作
    action_target_t* target = action_target_create(
        ACTION_TYPE_CALLBACK,
        DEVICE_TYPE_FLASH,
        0,
        0,
        0,
        0,
        NULL,  // 这里可以设置回调函数
        NULL
    );
    
    rules[0].targets = target;
    
    // 创建设备配置
    device_config_t config;
    config.mem_regions = mem_regions;
    config.region_count = 2;
    config.rules = rules;
    config.rule_count = 1;
    
    // 创建设备实例
    device_instance_t* flash = device_create_with_config(dm, DEVICE_TYPE_FLASH, 1, &config);
    
    // 注意：这里不需要释放target，因为它已经被添加到规则中
    
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
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("创建动作管理器失败\n");
        device_manager_destroy(dm);
        return 1;
    }
    
    // 创建全局监视器
    global_monitor_t* gm = global_monitor_create(am, dm);
    if (!gm) {
        printf("创建全局监视器失败\n");
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 注册测试规则提供者
    register_test_rule_provider();
    
    // 注册设备类型
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops());
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops());
    register_flash_device_type(dm);
    
    printf("设备注册表已初始化，包含 %d 种设备类型\n", MAX_DEVICE_TYPES);
    
    // 创建设备实例
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 0);
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);
    
    if (!temp_sensor || !fpga || !flash) {
        printf("创建设备实例失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 创建带配置的Flash设备
    device_instance_t* configured_flash = create_configured_flash_device(dm);
    if (configured_flash) {
        printf("成功创建带配置的Flash设备，ID: %d\n", configured_flash->dev_id);
    } else {
        printf("创建带配置的Flash设备失败\n");
    }
    
    // 加载所有规则
    printf("\n加载动作规则...\n");
    if (action_manager_load_all_rules(am) != 0) {
        printf("加载动作规则失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 运行所有设备测试
    if (run_all_device_tests(dm, gm, am) != 0) {
        printf("设备测试失败\n");
    } else {
        printf("所有设备测试通过\n");
    }
    
    // 清理资源
    printf("\n清理资源...\n");
    global_monitor_destroy(gm);
    action_manager_destroy(am);
    device_manager_destroy(dm);
    
    return 0;
}