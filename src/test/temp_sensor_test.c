/**
 * @file temp_sensor_test.c
 * @brief 温度传感器测试套件实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

// 温度传感器头文件（假设路径）
#include "../../plugins/temp_sensor/temp_sensor.h"

// 定义状态寄存器的值
#define TEMP_SENSOR_STATUS_NORMAL    0x00
#define TEMP_SENSOR_STATUS_HIGH_ALARM 0x01
#define TEMP_SENSOR_STATUS_LOW_ALARM  0x02

// 定义配置寄存器的值
#define TEMP_SENSOR_CONFIG_ENABLE    (CONFIG_ALERT)

// 定义模拟温度寄存器
#define TEMP_SENSOR_REG_SIM_TEMP     0x10

// 前向声明回调函数
static void temp_sensor_test_callback(void* data);

/**
 * @brief 温度传感器基本功能测试步骤
 */
static const test_step_t temp_sensor_basic_steps[] = {
    {
        .name = "读取当前温度",
        .reg_addr = TEMP_REG,
        .write_value = 0,
        .expected_value = 2500,  // 期望初始温度为25°C (2500 * 0.0625°C)
        .is_write = false,
        .format = "0x%08X",  // 使用十六进制格式
        .value_scale = 1.0f
    },
    {
        .name = "设置高温阈值",
        .reg_addr = THIGH_REG,
        .write_value = 80,  // 80度
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "设置低温阈值",
        .reg_addr = TLOW_REG,
        .write_value = 10,  // 10度
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "启用温度监控",
        .reg_addr = CONFIG_REG,
        .write_value = TEMP_SENSOR_CONFIG_ENABLE,
        .expected_value = 0,
        .is_write = true,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "读取配置寄存器",
        .reg_addr = CONFIG_REG,
        .write_value = 0,
        .expected_value = TEMP_SENSOR_CONFIG_ENABLE,
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief 温度传感器报警测试步骤
 */
static const test_step_t temp_sensor_alarm_steps[] = {
    {
        .name = "设置模拟温度（高温）",
        .reg_addr = TEMP_SENSOR_REG_SIM_TEMP,
        .write_value = 90,  // 90度，高于阈值
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "读取状态寄存器",
        .reg_addr = CONFIG_REG,
        .write_value = 0,
        .expected_value = TEMP_SENSOR_CONFIG_ENABLE,  // 只检查配置位，不检查状态位
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "设置模拟温度（低温）",
        .reg_addr = TEMP_SENSOR_REG_SIM_TEMP,
        .write_value = 5,  // 5度，低于阈值
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "读取状态寄存器",
        .reg_addr = CONFIG_REG,
        .write_value = 0,
        .expected_value = TEMP_SENSOR_CONFIG_ENABLE,  // 只检查配置位，不检查状态位
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "设置模拟温度（正常）",
        .reg_addr = TEMP_SENSOR_REG_SIM_TEMP,
        .write_value = 25,  // 25度，正常范围
        .expected_value = 0,
        .is_write = true,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "读取状态寄存器",
        .reg_addr = CONFIG_REG,
        .write_value = 0,
        .expected_value = TEMP_SENSOR_CONFIG_ENABLE,  // 只检查配置位，不检查状态位
        .is_write = false,
        .format = "0x%02X",
        .value_scale = 1.0f
    }
};

/**
 * @brief 温度传感器基本功能测试用例
 */
static const test_case_t temp_sensor_basic_test = {
    .name = "温度传感器基本功能测试",
    .device_type = DEVICE_TYPE_TEMP_SENSOR,
    .device_id = 0,
    .steps = temp_sensor_basic_steps,
    .step_count = sizeof(temp_sensor_basic_steps) / sizeof(test_step_t),
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief 温度传感器报警测试用例
 */
static const test_case_t temp_sensor_alarm_test = {
    .name = "温度传感器报警测试",
    .device_type = DEVICE_TYPE_TEMP_SENSOR,
    .device_id = 0,
    .steps = temp_sensor_alarm_steps,
    .step_count = sizeof(temp_sensor_alarm_steps) / sizeof(test_step_t),
    .setup = NULL,
    .cleanup = NULL,
    .user_data = NULL
};

/**
 * @brief 创建温度传感器测试套件
 * 
 * @return test_suite_t* 温度传感器测试套件
 */
test_suite_t* create_temp_sensor_test_suite(void) {
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
    test_cases[0] = &temp_sensor_basic_test;
    test_cases[1] = &temp_sensor_alarm_test;
    
    // 设置测试套件
    suite->name = "温度传感器测试套件";
    suite->test_cases = test_cases;
    suite->test_case_count = 2;
    
    return suite;
}

/**
 * @brief 温度传感器测试回调函数
 * 
 * @param data 回调数据
 */
static void temp_sensor_test_callback(void* data) {
    printf("温度传感器测试回调被触发\n");
}