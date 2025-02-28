#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "temp_sensor.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/device_memory.h"
#include "../include/device_rule_configs.h"

// 注册温度传感器设备
REGISTER_DEVICE(DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops);

// 温度报警回调函数
static void temp_alert_callback(void* data) {
    if (data) {
        printf("Temperature alert triggered! Current temperature: %.2f°C\n", 
            (*(uint32_t*)data) * 0.0625);
    }
}


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
    static device_ops_t ops = {
        .init = temp_sensor_init,
        .destroy = temp_sensor_destroy,
        .read = temp_sensor_read,
        .write = temp_sensor_write,
        .read_buffer = temp_sensor_read_buffer,
        .write_buffer = temp_sensor_write_buffer,
        .reset = temp_sensor_reset,
        .get_rule_manager = temp_sensor_get_rule_manager,
        .configure_memory = temp_sensor_configure_memory
    };
    return &ops;
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
    
    // 分配设备私有数据
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)calloc(1, sizeof(temp_sensor_data_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 创建设备内存（使用全局配置）
    dev_data->memory = device_memory_create(
        temp_sensor_memory_regions, 
        TEMP_SENSOR_REGION_COUNT, 
        NULL, 
        DEVICE_TYPE_TEMP_SENSOR, 
        instance->dev_id
    );
    
    if (!dev_data->memory) {
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 初始化规则计数器
    dev_data->rule_count = 0;
    
    // 初始化设备规则
    device_rule_manager_t* rule_manager = temp_sensor_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_TEMP_SENSOR);
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取温度传感器寄存器
static int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入温度传感器寄存器
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位温度传感器 - 简化为空函数
static void temp_sensor_reset(device_instance_t* instance) {
    // 不执行任何操作，保持接口兼容性
    (void)instance;
}

// 销毁温度传感器
static void temp_sensor_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 不再需要停止温度更新线程
    
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