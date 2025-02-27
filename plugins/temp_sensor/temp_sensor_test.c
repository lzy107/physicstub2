// temp_sensor_test.c
#include <stdio.h>
#include <stdlib.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/action_manager.h"
#include "../../include/global_monitor.h"
#include "temp_sensor.h"

// 温度报警回调函数
void temp_alert_callback(void* data) {
    (void)data; // 避免未使用参数警告
    printf("Temperature alert triggered!\n");
}

// 温度传感器测试步骤
static const test_step_t temp_sensor_steps[] = {
    {
        .name = "读取当前温度",
        .reg_addr = TEMP_REG,
        .is_write = 0,
        .format = "%d°C",
        .value_scale = 1.0f
    },
    {
        .name = "配置高温报警阈值",
        .reg_addr = THIGH_REG,
        .write_value = 0x50, // 80°C
        .is_write = 1,
        .format = "%d°C",
        .value_scale = 1.0f
    },
    {
        .name = "配置低温报警阈值",
        .reg_addr = TLOW_REG,
        .write_value = 0x20, // 32°C
        .is_write = 1,
        .format = "%d°C",
        .value_scale = 1.0f
    },
    {
        .name = "模拟温度变化触发报警",
        .reg_addr = TEMP_REG,
        .write_value = 0x60, // 96°C，超过报警阈值
        .is_write = 1,
        .format = "%d°C",
        .value_scale = 1.0f
    },
    {
        .name = "检查配置寄存器报警状态",
        .reg_addr = CONFIG_REG,
        .is_write = 0,
        .format = "0x%04X",
        .value_scale = 1.0f
    }
};

// 温度传感器测试用例
static const test_case_t temp_sensor_test_case = {
    .name = "温度传感器基本功能测试",
    .device_type = DEVICE_TYPE_TEMP_SENSOR,
    .device_id = 0,
    .steps = temp_sensor_steps,
    .step_count = sizeof(temp_sensor_steps) / sizeof(temp_sensor_steps[0]),
    .setup = NULL,
    .cleanup = NULL
};

// 设置温度传感器监视点和规则
void setup_temp_sensor_rules(global_monitor_t* gm, action_manager_t* am) {
    (void)am; // 避免未使用参数警告
    
    // 因使用全局规则配置，不再需要手动创建目标处理动作
    printf("温度传感器规则将通过全局规则配置加载，无需手动创建\n");
    
    // 如果需要添加额外的全局监视点，可以在这里添加
    // 例如：global_monitor_add_watch(gm, DEVICE_TYPE_TEMP_SENSOR, 0, STATUS_REG);
}

// 运行温度传感器测试
int run_temp_sensor_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am) {
    printf("\n=== 运行温度传感器测试 ===\n");
    
    // 设置温度传感器监视点
    global_monitor_add_watch(gm, DEVICE_TYPE_TEMP_SENSOR, 0, TEMP_REG);
    
    // 设置温度传感器规则
    setup_temp_sensor_rules(gm, am);
    
    // 运行测试用例
    return run_test_case(dm, &temp_sensor_test_case);
} 