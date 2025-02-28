// test_rule_provider.c
#include <stdio.h>
#include <stdlib.h>
#include "../../include/action_manager.h"
#include "../../include/device_types.h"
#include "../flash/flash_device.h"
#include "../temp_sensor/temp_sensor.h"
#include "test_rule_provider.h"

// 测试回调函数
void test_callback(void* data) {
    uint32_t* value = (uint32_t*)data;
    printf("Callback triggered with value: 0x%08X\n", *value);
}

// 测试规则
static rule_table_entry_t test_rules[2]; // 将在初始化时设置

static int test_rule_count = sizeof(test_rules) / sizeof(test_rules[0]);

// 获取测试规则
static const rule_table_entry_t* get_test_rules(void) {
    return test_rules;
}

// 获取测试规则数量
static int get_test_rule_count(void) {
    return test_rule_count;
}

// 测试规则提供者
static rule_provider_t test_provider = {
    .provider_name = "Test Provider",
    .get_rules = get_test_rules,
    .get_rule_count = get_test_rule_count
};

// 初始化测试规则
void init_test_rules(void) {
    // 创建规则时使用全局配置机制
    
    // 第一个规则 - 使用rule_trigger创建，而不是直接使用action_target_create
    rule_trigger_t trigger1 = rule_trigger_create(0x1000, 0x55AA, 0xFFFF);
    test_rules[0].name = "Test Rule 1";
    test_rules[0].trigger = trigger1;
    test_rules[0].targets = NULL;  // 目标将通过全局规则配置提供
    test_rules[0].priority = 100;
    
    // 第二个规则
    rule_trigger_t trigger2 = rule_trigger_create(CONFIG_REG, 0x0001, 0x0001);
    test_rules[1].name = "Test Rule 2";
    test_rules[1].trigger = trigger2;
    test_rules[1].targets = NULL;  // 目标将通过全局规则配置提供
    test_rules[1].priority = 50;
    
    printf("测试规则已初始化，将通过全局规则配置机制应用\n");
}

// 注册测试规则提供者
void register_test_rule_provider(void) {
    // 初始化测试规则
    init_test_rules();
    
    // 注册规则提供者
    action_manager_register_provider(&test_provider);
} 