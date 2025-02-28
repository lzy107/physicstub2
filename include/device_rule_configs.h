#ifndef DEVICE_RULE_CONFIGS_H
#define DEVICE_RULE_CONFIGS_H

#include <stdint.h>
#include "device_rules.h"
#include "action_manager.h"

// 设备规则配置结构体
typedef struct {
    uint32_t addr;                  // 监控地址
    uint32_t expected_value;        // 期望值
    uint32_t expected_mask;         // 掩码
    action_type_t action_type;      // 动作类型
    device_type_id_t target_device_type; // 目标设备类型
    int target_device_id;           // 目标设备ID
    uint32_t target_addr;           // 目标地址
    uint32_t target_value;          // 目标值
    uint32_t target_mask;           // 目标掩码
    action_callback_t callback;     // 回调函数
    void* callback_data;            // 回调数据
} device_rule_config_t;

// Flash设备规则配置
extern const device_rule_config_t flash_rule_configs[];
extern const int flash_rule_config_count;

// 温度传感器规则配置
extern const device_rule_config_t temp_sensor_rule_configs[];
extern const int temp_sensor_rule_config_count;

// FPGA设备规则配置
extern const device_rule_config_t fpga_rule_configs[];
extern const int fpga_rule_config_count;

// 从配置创建动作目标
action_target_t create_action_target_from_config(const device_rule_config_t* config);

// 从配置创建规则
device_rule_t create_device_rule_from_config(const device_rule_config_t* config);

// 获取设备规则配置
const rule_table_entry_t* get_device_rules(device_type_id_t device_type, int* count);

// 根据设备类型设置规则
int setup_device_rules(struct device_rule_manager* manager, device_type_id_t device_type);

#endif /* DEVICE_RULE_CONFIGS_H */ 