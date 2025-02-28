#include "device_rule_configs.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"
#include "device_rules.h"

// Flash 设备规则配置
const rule_table_entry_t flash_rules[] = {
    {
        .name = "Flash Status Done",
        .trigger = {
            .trigger_addr = FLASH_STATUS_REG,
            .expected_value = FLASH_STATUS_DONE,
            .expected_mask = FLASH_STATUS_DONE
        },
        .targets = {
            .targets = {
                {
                    .type = ACTION_TYPE_WRITE,
                    .device_type = DEVICE_TYPE_INTERRUPT_CONTROLLER,
                    .device_id = 0,
                    .target_addr = INTR_STATUS_REG,
                    .target_value = (1 << FLASH_INTR_BIT),
                    .target_mask = (1 << FLASH_INTR_BIT),
                    .callback = NULL,
                    .callback_data = NULL
                }
            },
            .count = 1
        },
        .priority = 1
    }
};
const int flash_rule_count = sizeof(flash_rules) / sizeof(flash_rules[0]);

// 温度传感器设备规则配置
const rule_table_entry_t temp_sensor_rules[] = {
    {
        .name = "Temperature High Alert",
        .trigger = {
            .trigger_addr = TEMP_SENSOR_STATUS_REG,
            .expected_value = TEMP_SENSOR_STATUS_ALERT,
            .expected_mask = TEMP_SENSOR_STATUS_ALERT
        },
        .targets = {
            .targets = {
                {
                    .type = ACTION_TYPE_WRITE,
                    .device_type = DEVICE_TYPE_INTERRUPT_CONTROLLER,
                    .device_id = 0,
                    .target_addr = INTR_STATUS_REG,
                    .target_value = (1 << TEMP_SENSOR_INTR_BIT),
                    .target_mask = (1 << TEMP_SENSOR_INTR_BIT),
                    .callback = NULL,
                    .callback_data = NULL
                }
            },
            .count = 1
        },
        .priority = 1
    }
};
const int temp_sensor_rule_count = sizeof(temp_sensor_rules) / sizeof(temp_sensor_rules[0]);

// FPGA 设备规则配置
const rule_table_entry_t fpga_rules[] = {
    {
        .name = "FPGA Configuration Complete",
        .trigger = {
            .trigger_addr = FPGA_STATUS_REG,
            .expected_value = FPGA_STATUS_CONFIGURED,
            .expected_mask = FPGA_STATUS_CONFIGURED
        },
        .targets = {
            .targets = {
                {
                    .type = ACTION_TYPE_WRITE,
                    .device_type = DEVICE_TYPE_INTERRUPT_CONTROLLER,
                    .device_id = 0,
                    .target_addr = INTR_STATUS_REG,
                    .target_value = (1 << FPGA_INTR_BIT),
                    .target_mask = (1 << FPGA_INTR_BIT),
                    .callback = NULL,
                    .callback_data = NULL
                }
            },
            .count = 1
        },
        .priority = 1
    }
};
const int fpga_rule_count = sizeof(fpga_rules) / sizeof(fpga_rules[0]);

// 获取设备规则配置
const rule_table_entry_t* get_device_rules(device_type_id_t device_type, int* count) {
    if (!count) return NULL;
    
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            *count = flash_rule_count;
            return flash_rules;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            *count = temp_sensor_rule_count;
            return temp_sensor_rules;
            
        case DEVICE_TYPE_FPGA:
            *count = fpga_rule_count;
            return fpga_rules;
            
        default:
            *count = 0;
            return NULL;
    }
}

// 为设备设置规则
int setup_device_rules(device_rule_manager_t* rule_manager, device_type_id_t device_type) {
    if (!rule_manager) return -1;
    
    int rule_count;
    const rule_table_entry_t* rules = get_device_rules(device_type, &rule_count);
    
    if (!rules || rule_count <= 0) return 0;
    
    for (int i = 0; i < rule_count; i++) {
        device_rule_add(rule_manager, 
                       rules[i].trigger.trigger_addr,
                       rules[i].trigger.expected_value,
                       rules[i].trigger.expected_mask,
                       &rules[i].targets);
    }
    
    return rule_count;
} 