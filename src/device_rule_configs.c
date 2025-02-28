#include "device_rule_configs.h"
#include "device_rules.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

// 外部声明各设备的规则配置
extern const device_rule_config_t flash_rule_configs[];
extern const int flash_rule_count;

extern const device_rule_config_t temp_sensor_rule_configs[];
extern const int temp_sensor_rule_count;

extern const device_rule_config_t fpga_rule_configs[];
extern const int fpga_rule_count;

// 根据设备类型设置规则
int setup_device_rules(device_rule_manager_t* manager, device_type_id_t device_type) {
    if (!manager) return 0;
    
    const device_rule_config_t* configs = NULL;
    int config_count = 0;
    
    // 根据设备类型选择规则配置
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            configs = flash_rule_configs;
            config_count = flash_rule_count;
            break;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            configs = temp_sensor_rule_configs;
            config_count = temp_sensor_rule_count;
            break;
            
        case DEVICE_TYPE_FPGA:
            configs = fpga_rule_configs;
            config_count = fpga_rule_count;
            break;
            
        default:
            return 0;
    }
    
    // 应用规则配置
    int rule_count = 0;
    for (int i = 0; i < config_count; i++) {
        action_target_t target;
        target.action_type = configs[i].action_type;
        target.device_type = configs[i].target_device_type;
        target.device_id = configs[i].target_device_id;
        target.addr = configs[i].target_addr;
        target.value = configs[i].target_value;
        target.callback = configs[i].callback;
        
        if (device_rule_add(manager, configs[i].addr, configs[i].expected_value, 
                           configs[i].expected_mask, &target) == 0) {
            rule_count++;
        }
    }
    
    return rule_count;
} 