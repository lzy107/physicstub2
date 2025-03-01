#include "../../include/device_rule_configs.h"
#include "flash_device.h"

// Flash 设备规则配置
const device_rule_config_t flash_rule_configs[] = {
    // 示例规则：当写入控制寄存器时触发操作
    {
        .addr = 0,
        .expected_value = 6,
        .expected_mask = 0xf,
        .action_type = ACTION_TYPE_WRITE,
        .target_device_type = DEVICE_TYPE_FLASH,
        .target_device_id = 0,  // 同一设备
        .target_addr = 5,       // 不适用于回调
        .target_value = 1,      // 不适用于回调
        .target_mask = 0xFF,    // 完全匹配
        .callback = NULL,
        .callback_data = NULL
    }
};
const int flash_rule_config_count = sizeof(flash_rule_configs) / sizeof(flash_rule_configs[0]); 