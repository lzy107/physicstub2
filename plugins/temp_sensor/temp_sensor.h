#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>
#include <pthread.h>
#include "device_types.h"
#include "device_memory.h"
#include "device_rules.h"
#include "action_manager.h"

// 设备类型ID
#define DEVICE_TYPE_TEMP_SENSOR 1

// 寄存器地址定义
#define TEMP_REG         0x00    // 温度寄存器 (R)
#define CONFIG_REG       0x01    // 配置寄存器 (R/W)
#define TLOW_REG        0x02    // 低温报警寄存器 (R/W)
#define THIGH_REG       0x03    // 高温报警寄存器 (R/W)

// 配置寄存器位定义
#define CONFIG_SHUTDOWN  (1 << 0)  // 关断模式
#define CONFIG_ALERT    (1 << 1)   // 报警输出使能
#define CONFIG_POLARITY (1 << 2)   // 报警极性
#define CONFIG_FQUEUE   (3 << 3)   // 故障队列
#define CONFIG_RES     (3 << 5)    // 转换分辨率
#define CONFIG_ONESHOT (1 << 7)    // 单次转换

// 温度传感器内存区域配置
#define TEMP_REG_REGION    0  // 寄存器区域索引
#define TEMP_REGION_COUNT  1  // 内存区域总数

// 温度传感器私有数据结构
typedef struct {
    device_instance_t base;       // 基础设备实例
    device_memory_t* memory;      // 设备内存
    pthread_mutex_t mutex;        // 互斥锁
    
    // 设备特定规则
    device_rule_t device_rules[8];    // 支持最多8个内置规则
    int rule_count;                   // 当前规则数量
    device_rule_manager_t rule_manager; // 规则管理器
} temp_sensor_device_t;

// 获取温度传感器操作接口
device_ops_t* get_temp_sensor_ops(void);

// 注册温度传感器设备类型
void register_temp_sensor_device_type(device_manager_t* dm);

// 向温度传感器添加规则
int temp_sensor_add_rule(device_instance_t* instance, uint32_t addr, 
                        uint32_t expected_value, uint32_t expected_mask, 
                        const action_target_array_t* targets);

// 函数声明
int temp_sensor_init(device_instance_t* instance);
void temp_sensor_destroy(device_instance_t* instance);
int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value);
int temp_sensor_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length);
int temp_sensor_write_buffer(device_instance_t* instance, uint32_t addr, const uint8_t* buffer, size_t length);
int temp_sensor_reset(device_instance_t* instance);
struct device_rule_manager* temp_sensor_get_rule_manager(device_instance_t* instance);
int temp_sensor_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count);

// 回调函数
void temp_alert_callback(void* context, uint32_t addr, uint32_t value);
void temp_config_callback(void* context, uint32_t addr, uint32_t value);

#endif // TEMP_SENSOR_H 