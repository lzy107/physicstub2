/**
 * @file flash_test.c
 * @brief Flash设备测试套件实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

// Flash设备头文件（假设路径）
#include "../../plugins/flash/flash_device.h"

// 前向声明回调函数
static void flash_test_callback(void* data);

/**
 * @brief Flash设备基本功能测试步骤
 */
static const test_step_t flash_basic_steps[] = {
    {
        .name = "设置写使能",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = FLASH_STATUS_WEL,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "设置地址寄存器",
        .reg_addr = FLASH_REG_ADDRESS,
        .write_value = 0x1000,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "写入数据寄存器",
        .reg_addr = FLASH_REG_DATA,
        .write_value = 0xABCD1234,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "执行写入操作",
        .reg_addr = FLASH_REG_CONTROL,
        .write_value = FLASH_CTRL_WRITE,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取状态寄存器",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0,
        .expected_value = FLASH_STATUS_READY,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "设置地址寄存器（读取）",
        .reg_addr = FLASH_REG_ADDRESS,
        .write_value = 0x1000,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "执行读取操作",
        .reg_addr = FLASH_REG_CONTROL,
        .write_value = FLASH_CTRL_READ,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取数据寄存器",
        .reg_addr = FLASH_REG_DATA,
        .write_value = 0,
        .expected_value = 0xABCD1234,
        .is_write = false,
        .format = "0x%08X",
        .value_scale = 1.0f
    }
};

/**
 * @brief Flash设备规则触发测试步骤
 */
static const test_step_t flash_rule_steps[] = {
    {
        .name = "触发状态寄存器规则",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0x01,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "验证状态寄存器变化",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0,
        .expected_value = 0x03,  // 假设规则会设置额外的状态位
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief Flash设备错误处理测试步骤
 */
static const test_step_t flash_error_steps[] = {
    {
        .name = "不设置写使能直接写入",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0x00,  // 清除写使能
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "尝试写入数据",
        .reg_addr = FLASH_REG_DATA,
        .write_value = 0x12345678,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "执行写入操作",
        .reg_addr = FLASH_REG_CONTROL,
        .write_value = FLASH_CTRL_WRITE,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "检查状态寄存器错误标志",
        .reg_addr = FLASH_REG_STATUS,
        .write_value = 0,
        .expected_value = FLASH_STATUS_ERROR,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief Flash基本功能测试用例
 */
static const test_case_t flash_basic_test = {
    .name = "Flash设备基本功能测试",
    .device_type = DEVICE_TYPE_FLASH,
    .device_id = 0,
    .steps = flash_basic_steps,
    .step_count = sizeof(flash_basic_steps) / sizeof(test_step_t),
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief Flash规则触发测试用例
 */
static const test_case_t flash_rule_test = {
    .name = "Flash设备规则触发测试",
    .device_type = DEVICE_TYPE_FLASH,
    .device_id = 0,
    .steps = flash_rule_steps,
    .step_count = sizeof(flash_rule_steps) / sizeof(test_step_t),
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief Flash错误处理测试用例
 */
static const test_case_t flash_error_test = {
    .name = "Flash设备错误处理测试",
    .device_type = DEVICE_TYPE_FLASH,
    .device_id = 0,
    .steps = flash_error_steps,
    .step_count = sizeof(flash_error_steps) / sizeof(test_step_t),
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief 创建Flash测试套件
 * 
 * @return test_suite_t* Flash测试套件
 */
test_suite_t* create_flash_test_suite(void) {
    test_suite_t* suite = (test_suite_t*)malloc(sizeof(test_suite_t));
    if (!suite) {
        printf("错误: 内存分配失败\n");
        return NULL;
    }
    
    // 分配测试用例数组
    const test_case_t** test_cases = (const test_case_t**)malloc(3 * sizeof(test_case_t*));
    if (!test_cases) {
        printf("错误: 内存分配失败\n");
        free(suite);
        return NULL;
    }
    
    // 设置测试用例
    test_cases[0] = &flash_basic_test;
    test_cases[1] = &flash_rule_test;
    test_cases[2] = &flash_error_test;
    
    // 设置测试套件
    suite->name = "Flash设备测试套件";
    suite->test_cases = test_cases;
    suite->test_case_count = 3;
    
    return suite;
}

/**
 * @brief 销毁测试套件
 * 
 * @param suite 测试套件
 */
void destroy_test_suite(test_suite_t* suite) {
    if (suite) {
        if (suite->test_cases) {
            free((void*)suite->test_cases);
        }
        free(suite);
    }
}

/**
 * @brief Flash测试回调函数
 * 
 * @param data 回调数据
 */
static void flash_test_callback(void* data) {
    uint32_t* value = (uint32_t*)data;
    printf("Flash回调触发，值: 0x%08X\n", *value);
} 