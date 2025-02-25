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
#include "../plugins/flash_device.h"
#include "../plugins/fpga_device.h"
#include "../plugins/temp_sensor.h"

// 测试回调函数
void test_callback(void* data) {
    uint32_t* value = (uint32_t*)data;
    printf("Callback triggered with value: 0x%08X\n", *value);
}

// 温度报警回调函数
void temp_alert_callback(void* data) {
    (void)data; // 避免未使用参数警告
    printf("Temperature alert triggered!\n");
}

// FPGA 中断回调函数
void fpga_irq_callback(void* data) {
    (void)data; // 避免未使用参数警告
    printf("FPGA interrupt triggered!\n");
}

// 测试规则提供者
static rule_table_entry_t test_rules[2]; // 将在初始化时设置

static int test_rule_count = sizeof(test_rules) / sizeof(test_rules[0]);

static const rule_table_entry_t* get_test_rules(void) {
    return test_rules;
}

static int get_test_rule_count(void) {
    return test_rule_count;
}

static rule_provider_t test_provider = {
    .provider_name = "Test Provider",
    .get_rules = get_test_rules,
    .get_rule_count = get_test_rule_count
};

// 初始化测试规则
static void init_test_rules(void) {
    // 创建第一个规则的目标处理动作
    action_target_t* target1 = action_target_create(
        ACTION_TYPE_CALLBACK, 
        DEVICE_TYPE_FLASH, // 使用FLASH设备类型代替NONE
        0, 
        0, 
        0, 
        0, 
        test_callback, 
        NULL
    );
    
    // 创建第一个规则
    rule_trigger_t trigger1 = rule_trigger_create(0x1000, 0x55AA, 0xFFFF);
    test_rules[0].name = "Test Rule 1";
    test_rules[0].trigger = trigger1;
    test_rules[0].targets = target1;
    test_rules[0].priority = 100;
    
    // 创建第二个规则的目标处理动作
    action_target_t* target2 = action_target_create(
        ACTION_TYPE_CALLBACK, 
        DEVICE_TYPE_FLASH, // 使用FLASH设备类型代替NONE
        0, 
        0, 
        0, 
        0, 
        temp_alert_callback, 
        NULL
    );
    
    // 添加第二个目标处理动作（写入操作）
    action_target_t* target3 = action_target_create(
        ACTION_TYPE_WRITE, 
        DEVICE_TYPE_TEMP_SENSOR, 
        0, 
        CONFIG_REG, // 使用CONFIG_REG代替TEMP_REG_CONFIG
        0x0001,  // 设置报警标志
        0x0001, 
        NULL, 
        NULL
    );
    
    // 将目标处理动作链接起来
    action_target_add(&target2, target3);
    
    // 创建第二个规则
    rule_trigger_t trigger2 = rule_trigger_create(TEMP_REG, 0x0050, 0x00FF); // 使用TEMP_REG代替TEMP_REG_TEMP
    test_rules[1].name = "Temperature Alert Rule";
    test_rules[1].trigger = trigger2;
    test_rules[1].targets = target2;
    test_rules[1].priority = 200;
}

int main() {
    printf("Initializing device simulator...\n");
    
    // 初始化测试规则
    init_test_rules();
    
    // 创建设备管理器
    device_manager_t* dm = device_manager_init(); // 使用device_manager_init代替device_manager_create
    if (!dm) {
        printf("Failed to create device manager\n");
        return 1;
    }
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("Failed to create action manager\n");
        device_manager_destroy(dm);
        return 1;
    }
    
    // 创建全局监视器
    global_monitor_t* gm = global_monitor_create(am, dm);
    if (!gm) {
        printf("Failed to create global monitor\n");
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 注册规则提供者
    action_manager_register_provider(&test_provider);
    
    // 注册设备类型
    // 使用正确的函数名称和参数
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops());
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops());
    register_flash_device_type(dm);
    
    printf("Device registry initialized with %d device types\n", MAX_DEVICE_TYPES);
    
    // 创建设备实例，移除多余的参数
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 0);
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);
    
    if (!temp_sensor || !fpga || !flash) {
        printf("Failed to create device instances\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 加载所有规则
    printf("\nLoading action rules...\n");
    if (action_manager_load_all_rules(am) != 0) {
        printf("Failed to load action rules\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 设置监视点
    printf("\nSetting up watch points...\n");
    global_monitor_add_watch(gm, DEVICE_TYPE_TEMP_SENSOR, 0, TEMP_REG); // 使用TEMP_REG代替TEMP_REG_TEMP
    global_monitor_add_watch(gm, DEVICE_TYPE_FPGA, 0, 0x1000);
    
    // 测试 FLASH 设备
    printf("\nTesting FLASH device...\n");
    uint32_t data = 0xABCD1234;
    device_ops_t* flash_ops = &dm->types[DEVICE_TYPE_FLASH].ops;
    flash_ops->write(flash, FLASH_REG_DATA, data);
    
    uint32_t read_data = 0;
    flash_ops->read(flash, FLASH_REG_DATA, &read_data);
    printf("FLASH read: 0x%08X\n", read_data);
    
    // 测试温度传感器
    printf("\nTesting temperature sensor...\n");
    device_ops_t* temp_ops = &dm->types[DEVICE_TYPE_TEMP_SENSOR].ops;
    
    uint32_t temp_value = 0;
    temp_ops->read(temp_sensor, TEMP_REG, &temp_value); // 使用TEMP_REG代替TEMP_REG_TEMP
    printf("Current temperature: %d°C\n", temp_value & 0xFF);
    
    // 配置温度报警
    temp_ops->write(temp_sensor, THIGH_REG, 0x50);  // 80°C，使用THIGH_REG代替TEMP_REG_TEMP_HIGH
    temp_ops->write(temp_sensor, TLOW_REG, 0x20);   // 32°C，使用TLOW_REG代替TEMP_REG_TEMP_LOW
    
    // 测试温度变化触发规则
    printf("\nTesting temperature alert...\n");
    temp_ops->write(temp_sensor, TEMP_REG, 0x60);  // 96°C，超过报警阈值，使用TEMP_REG代替TEMP_REG_TEMP
    
    uint32_t config_value = 0;
    temp_ops->read(temp_sensor, CONFIG_REG, &config_value); // 使用CONFIG_REG代替TEMP_REG_CONFIG
    printf("Temperature sensor config after alert: 0x%04X\n", config_value);
    
    // 测试 FPGA 设备
    printf("\nTesting FPGA device...\n");
    device_ops_t* fpga_ops = &dm->types[DEVICE_TYPE_FPGA].ops;
    
    // 写入触发规则的值
    fpga_ops->write(fpga, 0x1000, 0x55AA);
    
    // 读取 FPGA 内存
    uint32_t fpga_data = 0;
    fpga_ops->read(fpga, 0x1000, &fpga_data);
    printf("FPGA read: 0x%08X\n", fpga_data);
    
    // 测试自定义规则
    printf("\nTesting custom rule...\n");
    
    // 创建目标处理动作
    action_target_t* custom_target = action_target_create(
        ACTION_TYPE_WRITE, 
        DEVICE_TYPE_FPGA, 
        0, 
        0x2000, 
        0xDEADBEEF, 
        0xFFFFFFFF, 
        NULL, 
        NULL
    );
    
    // 创建触发条件
    rule_trigger_t custom_trigger = rule_trigger_create(FLASH_REG_STATUS, 0x01, 0x01);
    
    // 设置监视点和规则
    global_monitor_setup_watch_rule(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_STATUS, 
                                   0x01, 0x01, custom_target);
    
    // 触发规则
    flash_ops->write(flash, FLASH_REG_STATUS, 0x01);
    
    // 检查结果
    uint32_t result_data = 0;
    fpga_ops->read(fpga, 0x2000, &result_data);
    printf("FPGA memory after rule trigger: 0x%08X\n", result_data);
    
    // 清理资源
    printf("\nCleaning up...\n");
    global_monitor_destroy(gm);
    action_manager_destroy(am);
    device_manager_destroy(dm);
    
    printf("All tests completed successfully\n");
    return 0;
}