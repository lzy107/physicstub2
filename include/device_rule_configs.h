#ifndef DEVICE_RULE_CONFIGS_H
#define DEVICE_RULE_CONFIGS_H

#include "action_manager.h"
#include "device_types.h"
#include "device_rules.h"

// Flash 设备规则配置
extern const rule_table_entry_t flash_rules[];
extern const int flash_rule_count;

// 温度传感器设备规则配置
extern const rule_table_entry_t temp_sensor_rules[];
extern const int temp_sensor_rule_count;

// FPGA 设备规则配置
extern const rule_table_entry_t fpga_rules[];
extern const int fpga_rule_count;

// 获取设备规则配置
const rule_table_entry_t* get_device_rules(device_type_id_t device_type, int* count);

// 为设备设置规则
int setup_device_rules(device_rule_manager_t* rule_manager, device_type_id_t device_type);

// 各设备规则配置的外部声明
extern const device_rule_config_t flash_rule_configs[];
extern const int flash_rule_count;

extern const device_rule_config_t temp_sensor_rule_configs[];
extern const int temp_sensor_rule_count;

extern const device_rule_config_t fpga_rule_configs[];
extern const int fpga_rule_count;

#endif /* DEVICE_RULE_CONFIGS_H */ 