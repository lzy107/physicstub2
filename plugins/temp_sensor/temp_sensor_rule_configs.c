#include "../../include/device_rule_configs.h"
#include "temp_sensor.h"
#include "../../include/action_manager.h"

// 温度传感器设备规则配置
const device_rule_config_t temp_sensor_rule_configs[] = {
    // 示例规则：当温度超过阈值时触发操作
    {
        .addr = 0x4,
        .expected_value = 3,  // 任意值
        .expected_mask = 0xf,   // 不检查具体值
        .target_mask = 0xFF,           // 完全匹配
        .action_type = ACTION_TYPE_WRITE,
        .target_device_type = DEVICE_TYPE_TEMP_SENSOR,
        .target_device_id = 0,  // 同一设备
        .target_addr = 0x8,       // 不适用于回调
        .target_value = 0x5,      // 不适用于回调
        .callback = NULL
    }
};
const int temp_sensor_rule_config_count = sizeof(temp_sensor_rule_configs) / sizeof(temp_sensor_rule_configs[0]); 