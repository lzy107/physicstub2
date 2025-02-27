#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "temp_sensor.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/device_memory.h"

// 注册温度传感器设备
REGISTER_DEVICE(DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops);

// 温度报警回调函数
static void temp_alert_callback(void* data) {
    if (data) {
        printf("Temperature alert triggered! Current temperature: %.2f°C\n", 
            (*(uint32_t*)data) * 0.0625);
    }
}

// 温度传感器规则表
static rule_table_entry_t temp_sensor_rules[1];

// 初始化温度传感器规则表
static void init_temp_sensor_rules(void) {
    // 创建目标处理动作
    action_target_t* target = action_target_create(
        ACTION_TYPE_CALLBACK,
        DEVICE_TYPE_TEMP_SENSOR,
        0,
        0,
        0,
        0,
        temp_alert_callback,
        NULL
    );
    
    // 创建规则触发条件
    rule_trigger_t trigger = rule_trigger_create(TEMP_REG, 3000, 0xFFFF);
    
    // 设置规则
    temp_sensor_rules[0].name = "Temperature High Alert";
    temp_sensor_rules[0].trigger = trigger;
    temp_sensor_rules[0].targets = target;
    temp_sensor_rules[0].priority = 1;
}

// 获取温度传感器规则表
static const rule_table_entry_t* get_temp_sensor_rules(void) {
    return temp_sensor_rules;
}

// 获取温度传感器规则数量
static int get_temp_sensor_rule_count(void) {
    return sizeof(temp_sensor_rules) / sizeof(temp_sensor_rules[0]);
}

// 规则提供者
static rule_provider_t temp_sensor_rule_provider = {
    .provider_name = "Temperature Sensor Rules",
    .get_rules = get_temp_sensor_rules,
    .get_rule_count = get_temp_sensor_rule_count
};

// 私有函数声明
static int temp_sensor_init(device_instance_t* instance);
static int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void temp_sensor_reset(device_instance_t* instance);
static void temp_sensor_destroy(device_instance_t* instance);

// 获取温度传感器互斥锁
static pthread_mutex_t* temp_sensor_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    return &dev_data->mutex;
}

// 获取温度传感器规则管理器
static device_rule_manager_t* temp_sensor_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    
    // 使用静态变量存储规则管理器，避免每次调用都分配内存
    static device_rule_manager_t rule_manager;
    
    // 初始化规则管理器
    rule_manager.rules = dev_data->device_rules;
    rule_manager.rule_count = dev_data->rule_count;
    rule_manager.max_rules = 8; // 温度传感器支持8个规则
    rule_manager.mutex = &dev_data->mutex;
    
    return &rule_manager;
}

// 温度传感器操作接口
static device_ops_t temp_sensor_ops = {
    .init = temp_sensor_init,
    .read = temp_sensor_read,
    .write = temp_sensor_write,
    .reset = temp_sensor_reset,
    .destroy = temp_sensor_destroy,
    .get_mutex = temp_sensor_get_mutex,
    .get_rule_manager = temp_sensor_get_rule_manager
};

// 获取温度传感器操作接口
device_ops_t* get_temp_sensor_ops(void) {
    return &temp_sensor_ops;
}

// 温度更新线程函数
static void* temp_update_thread(void* arg) {
    device_instance_t* instance = (device_instance_t*)arg;
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    uint32_t value;
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 读取配置寄存器
        if (device_memory_read(dev_data->memory, CONFIG_REG, &value) == 0) {
            // 如果不在关断模式
            if (!(value & CONFIG_SHUTDOWN)) {
                // 模拟温度变化（在20-30度之间随机波动）
                uint32_t temp = 2000 + (rand() % 1000);  // 0.0625°C/bit
                device_memory_write(dev_data->memory, TEMP_REG, temp);
                
                // 检查报警条件
                if (value & CONFIG_ALERT) {
                    uint32_t tlow, thigh;
                    if (device_memory_read(dev_data->memory, TLOW_REG, &tlow) == 0 &&
                        device_memory_read(dev_data->memory, THIGH_REG, &thigh) == 0) {
                        // 更新报警状态
                        if (temp >= thigh || temp <= tlow) {
                            value |= (1 << 15);  // 设置报警位
                        } else {
                            value &= ~(1 << 15);  // 清除报警位
                        }
                        device_memory_write(dev_data->memory, CONFIG_REG, value);
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&dev_data->mutex);
        usleep(100000);  // 100ms更新一次
    }
    
    return NULL;
}

// 初始化温度传感器
static int temp_sensor_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 初始化规则表
    init_temp_sensor_rules();
    
    // 注册规则提供者
    action_manager_register_provider(&temp_sensor_rule_provider);
    
    // 分配设备私有数据
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)calloc(1, sizeof(temp_sensor_data_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->running = 1;
    
    // 初始化规则计数器
    dev_data->rule_count = 0;
    
    // 定义内存区域 - 温度传感器只有一个寄存器区域
    memory_region_t regions[1];
    
    // 寄存器区域
    regions[0].base_addr = 0x00;
    regions[0].unit_size = 4;  // 4字节单位
    regions[0].length = 4;     // 4个寄存器
    
    // 创建设备内存
    dev_data->memory = device_memory_create(regions, 1, NULL, DEVICE_TYPE_TEMP_SENSOR, instance->dev_id);
    if (!dev_data->memory) {
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 设置初始值
    uint32_t temp = 2500;    // 25°C
    uint32_t tlow = 1800;    // 18°C
    uint32_t thigh = 3000;   // 30°C
    uint32_t config = 0;     // 正常模式
    
    device_memory_write(dev_data->memory, TEMP_REG, temp);
    device_memory_write(dev_data->memory, TLOW_REG, tlow);
    device_memory_write(dev_data->memory, THIGH_REG, thigh);
    device_memory_write(dev_data->memory, CONFIG_REG, config);
    
    // 启动温度更新线程
    if (pthread_create(&dev_data->update_thread, NULL, temp_update_thread, instance) != 0) {
        device_memory_destroy(dev_data->memory);
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取温度传感器寄存器
static int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    int ret = device_memory_read(dev_data->memory, addr, value);
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 写入温度传感器寄存器
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    int ret = device_memory_write(dev_data->memory, addr, value);
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 复位温度传感器
static void temp_sensor_reset(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 复位所有寄存器到默认值
    uint32_t temp = 2500;    // 25°C
    uint32_t tlow = 1800;    // 18°C
    uint32_t thigh = 3000;   // 30°C
    uint32_t config = 0;     // 正常模式
    
    device_memory_write(dev_data->memory, TEMP_REG, temp);
    device_memory_write(dev_data->memory, TLOW_REG, tlow);
    device_memory_write(dev_data->memory, THIGH_REG, thigh);
    device_memory_write(dev_data->memory, CONFIG_REG, config);
    
    pthread_mutex_unlock(&dev_data->mutex);
}

// 销毁温度传感器
static void temp_sensor_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 停止温度更新线程
    dev_data->running = 0;
    pthread_join(dev_data->update_thread, NULL);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_memory_destroy(dev_data->memory);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}

// 注册温度传感器设备类型
void register_temp_sensor_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_temp_sensor_ops();
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", ops);
}

// 向温度传感器添加规则
int temp_sensor_add_rule(device_instance_t* instance, uint32_t addr, 
                        uint32_t expected_value, uint32_t expected_mask, 
                        action_target_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    
    // 使用通用的规则添加函数
    device_rule_manager_t manager;
    device_rule_manager_init(&manager, &dev_data->mutex, dev_data->device_rules, 8);
    manager.rule_count = dev_data->rule_count;
    
    int result = device_rule_add(&manager, addr, expected_value, expected_mask, targets);
    
    // 更新规则计数
    dev_data->rule_count = manager.rule_count;
    
    return result;
} 