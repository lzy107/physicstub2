// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "device_types.h"
#include "global_monitor.h"
#include "action_manager.h"
#include "device_registry.h"
#include "device_test.h"
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

// 通用测试规则表
static const rule_table_entry_t test_rules[] = {
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

// 获取测试规则表
static const rule_table_entry_t* get_test_rules(void) {
    return test_rules;
}

// 获取测试规则数量
static int get_test_rule_count(void) {
    return sizeof(test_rules) / sizeof(test_rules[0]);
}

// 注册测试规则提供者
REGISTER_RULE_PROVIDER(test, get_test_rules, get_test_rule_count);

// FLASH设备测试步骤
static const test_step_t flash_test_steps[] = {
    {
        .name = "Setting write enable",
        .reg_addr = FLASH_STATUS_REG,
        .write_value = STATUS_WEL,
        .is_write = 1,
        .format = "0x%02X"
    },
    {
        .name = "Writing data to flash",
        .reg_addr = FLASH_DATA_START,
        .write_value = 0x55,
        .is_write = 1,
        .format = "0x%02X"
    },
    {
        .name = "Reading data from flash",
        .reg_addr = FLASH_DATA_START,
        .expect_value = 0x55,
        .is_write = 0,
        .format = "0x%02X"
    }
};

// 温度传感器测试步骤
static const test_step_t temp_sensor_test_steps[] = {
    {
        .name = "Reading current temperature",
        .reg_addr = TEMP_REG,
        .is_write = 0,
        .value_scale = 0.0625f,
        .format = "%.2f°C"
    },
    {
        .name = "Configuring temperature alerts",
        .reg_addr = CONFIG_REG,
        .write_value = CONFIG_ALERT,
        .is_write = 1,
        .format = "0x%02X"
    },
    {
        .name = "Setting low temperature threshold",
        .reg_addr = TLOW_REG,
        .write_value = 2000,  // 20°C
        .is_write = 1,
        .value_scale = 0.0625f,
        .format = "%.2f°C"
    },
    {
        .name = "Setting high temperature threshold",
        .reg_addr = THIGH_REG,
        .write_value = 2800,  // 28°C
        .is_write = 1,
        .value_scale = 0.0625f,
        .format = "%.2f°C"
    },
    {
        .name = "Monitoring temperature",
        .reg_addr = TEMP_REG,
        .is_write = 0,
        .value_scale = 0.0625f,
        .format = "%.2f°C"
    }
};

// FPGA设备测试步骤
static const test_step_t fpga_test_steps[] = {
    {
        .name = "Enabling FPGA and IRQ",
        .reg_addr = FPGA_CONFIG_REG,
        .write_value = CONFIG_ENABLE | CONFIG_IRQ_EN,
        .is_write = 1,
        .format = "0x%08X"
    },
    {
        .name = "Reading FPGA status",
        .reg_addr = FPGA_STATUS_REG,
        .is_write = 0,
        .format = "0x%08X"
    },
    {
        .name = "Starting FPGA operation",
        .reg_addr = FPGA_CONTROL_REG,
        .write_value = CTRL_START,
        .is_write = 1,
        .format = "0x%08X"
    },
    {
        .name = "Checking IRQ status",
        .reg_addr = FPGA_IRQ_REG,
        .is_write = 0,
        .format = "0x%08X"
    }
};

// 测试用例定义
static const test_case_t test_cases[] = {
    {
        .name = "FLASH Device Basic Operations",
        .device_type = DEVICE_TYPE_FLASH,
        .device_id = 1,
        .steps = flash_test_steps,
        .step_count = sizeof(flash_test_steps) / sizeof(flash_test_steps[0])
    },
    {
        .name = "Temperature Sensor Operations",
        .device_type = DEVICE_TYPE_TEMP_SENSOR,
        .device_id = 1,
        .steps = temp_sensor_test_steps,
        .step_count = sizeof(temp_sensor_test_steps) / sizeof(temp_sensor_test_steps[0])
    },
    {
        .name = "FPGA Device Operations",
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 1,
        .steps = fpga_test_steps,
        .step_count = sizeof(fpga_test_steps) / sizeof(fpga_test_steps[0])
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
    
    // 加载所有规则
    printf("\nLoading action rules...\n");
    if (action_manager_load_all_rules(am) != 0) {
        printf("Failed to load action rules\n");
        goto cleanup;
    }
    
    // 运行测试套件
    if (run_test_suite(dm, test_cases, sizeof(test_cases) / sizeof(test_cases[0])) != 0) {
        printf("Some tests failed!\n");
        goto cleanup;
    }
    
    printf("\nAll tests completed successfully!\n");
    
cleanup:
    // 清理资源
    printf("\nCleaning up resources...\n");
    
    if (gm) {
        printf("Destroying global monitor...\n");
        global_monitor_destroy(gm);
    }
    
    if (am) {
        printf("Destroying action manager...\n");
        action_manager_destroy(am);
    }
    
    if (dm) {
        printf("Destroying device manager...\n");
        device_manager_destroy(dm);
    }
    
    printf("Cleanup completed.\n");
    return 0;
}