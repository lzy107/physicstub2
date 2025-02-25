// flash_device_test.c
#include <stdio.h>
#include <stdlib.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/action_manager.h"
#include "../../include/global_monitor.h"
#include "flash_device.h"

// 测试回调函数
void flash_test_callback(void* data) {
    uint32_t* value = (uint32_t*)data;
    printf("Flash callback triggered with value: 0x%08X\n", *value);
}

// Flash设备测试步骤
static const test_step_t flash_steps[] = {
    {
        .name = "设置写使能",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = FLASH_STATUS_WEL,
        .is_write = 1,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "设置地址寄存器",
        .reg_addr = FLASH_REG_ADDRESS,
        .write_value = 0x1000,
        .is_write = 1,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "写入数据寄存器",
        .reg_addr = FLASH_REG_DATA,
        .write_value = 0xABCD1234,
        .is_write = 1,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "设置地址寄存器（再次）",
        .reg_addr = FLASH_REG_ADDRESS,
        .write_value = 0x1000,
        .is_write = 1,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "读取数据",
        .reg_addr = 0x1000,
        .is_write = 0,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "触发规则（写入状态寄存器）",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0x01,
        .is_write = 1,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取状态寄存器",
        .reg_addr = FLASH_REG_STATUS,
        .is_write = 0,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

// Flash设备测试用例
static const test_case_t flash_test_case = {
    .name = "Flash设备基本功能测试",
    .device_type = DEVICE_TYPE_FLASH,
    .device_id = 0,
    .steps = flash_steps,
    .step_count = sizeof(flash_steps) / sizeof(flash_steps[0]),
    .setup = NULL,
    .cleanup = NULL
};

// 设置Flash监视点和规则
void setup_flash_rules(global_monitor_t* gm, action_manager_t* am) {
    (void)am; // 避免未使用参数警告
    
    // 创建Flash规则的目标处理动作
    action_target_t* target = action_target_create(
        ACTION_TYPE_CALLBACK, 
        DEVICE_TYPE_FLASH, 
        0, 
        0, 
        0, 
        0, 
        flash_test_callback, 
        NULL
    );
    
    // 设置监视点和规则
    global_monitor_setup_watch_rule(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_DATA, 
                                   0xABCD1234, 0xFFFFFFFF, target);
}

// 运行Flash设备测试
int run_flash_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am) {
    printf("\n=== 运行Flash设备测试 ===\n");
    
    // 设置Flash监视点
    global_monitor_add_watch(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_STATUS);
    
    // 设置Flash规则
    setup_flash_rules(gm, am);
    
    // 运行测试用例
    return run_test_case(dm, &flash_test_case);
} 