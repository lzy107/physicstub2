/**
 * @file fpga_test.c
 * @brief FPGA设备测试套件实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

// FPGA设备头文件（假设路径）
#include "../../plugins/fpga/fpga_device.h"

// 前向声明回调函数
static void fpga_test_callback(void* data);

/**
 * @brief FPGA设备配置测试步骤
 */
static const test_step_t fpga_config_steps[] = {
    {
        .name = "设置配置寄存器",
        .reg_addr = FPGA_CONFIG_REG,
        .write_value = CONFIG_RESET,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取配置状态",
        .reg_addr = FPGA_STATUS_REG,
        .write_value = 0,
        .expected_value = STATUS_DONE,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief FPGA设备中断测试步骤
 */
static const test_step_t fpga_interrupt_steps[] = {
    {
        .name = "设置中断使能",
        .reg_addr = FPGA_CONFIG_REG,
        .write_value = CONFIG_IRQ_EN,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "触发中断",
        .reg_addr = FPGA_CONTROL_REG,
        .write_value = CTRL_START,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取中断状态",
        .reg_addr = FPGA_IRQ_REG,
        .write_value = 0,
        .expected_value = 0x01,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "清除中断",
        .reg_addr = FPGA_IRQ_REG,
        .write_value = 0x01,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "验证中断已清除",
        .reg_addr = FPGA_IRQ_REG,
        .write_value = 0,
        .expected_value = 0x00,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief FPGA配置测试用例
 */
static const test_case_t fpga_config_test = {
    .name = "FPGA设备配置测试",
    .device_type = DEVICE_TYPE_FPGA,
    .device_id = 0,
    .steps = fpga_config_steps,
    .step_count = 2,  // 固定数组大小
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief FPGA中断测试用例
 */
static const test_case_t fpga_interrupt_test = {
    .name = "FPGA设备中断测试",
    .device_type = DEVICE_TYPE_FPGA,
    .device_id = 0,
    .steps = fpga_interrupt_steps,
    .step_count = 5,  // 固定数组大小
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief 创建FPGA测试套件
 * 
 * @return test_suite_t* FPGA测试套件
 */
test_suite_t* create_fpga_test_suite(void) {
    test_suite_t* suite = (test_suite_t*)malloc(sizeof(test_suite_t));
    if (!suite) {
        printf("错误: 内存分配失败\n");
        return NULL;
    }
    
    // 分配测试用例数组
    const test_case_t** test_cases = (const test_case_t**)malloc(2 * sizeof(test_case_t*));
    if (!test_cases) {
        printf("错误: 内存分配失败\n");
        free(suite);
        return NULL;
    }
    
    // 设置测试用例
    test_cases[0] = &fpga_config_test;
    test_cases[1] = &fpga_interrupt_test;
    
    // 设置测试套件
    suite->name = "FPGA设备测试套件";
    suite->test_cases = test_cases;
    suite->test_case_count = 2;
    
    return suite;
}

/**
 * @brief FPGA测试回调函数
 * 
 * @param data 回调数据
 */
static void fpga_test_callback(void* data) {
    uint32_t* value = (uint32_t*)data;
    printf("FPGA回调触发，值: 0x%08X\n", *value);
} 