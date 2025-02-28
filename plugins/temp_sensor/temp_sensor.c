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
void temp_alert_callback(void* context, uint32_t addr, uint32_t value) {
    if (context) {
        printf("Temperature alert triggered! Current temperature: %.2f°C\n", 
            value * 0.0625);
    }
}

// 配置回调函数
void temp_config_callback(void* context, uint32_t addr, uint32_t value) {
    printf("Temperature sensor configuration changed: 0x%08x\n", value);
}

// 获取温度传感器互斥锁
static pthread_mutex_t* temp_sensor_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    return &dev_data->mutex;
}

// 获取温度传感器规则管理器
struct device_rule_manager* temp_sensor_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    
    // 初始化规则管理器
    dev_data->rule_manager.rules = dev_data->device_rules;
    dev_data->rule_manager.rule_count = dev_data->rule_count;
    
    return &dev_data->rule_manager;
}

// 温度更新线程函数
static void* temp_update_thread(void* arg) {
    device_instance_t* instance = (device_instance_t*)arg;
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
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
int temp_sensor_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)calloc(1, sizeof(temp_sensor_device_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 创建设备内存
    memory_region_t regions[TEMP_REGION_COUNT] = {
        {
            .base_addr = 0,
            .unit_size = 4,
            .length = 4,
            .data = NULL,
            .device_type = DEVICE_TYPE_TEMP_SENSOR,
            .device_id = instance->dev_id
        }
    };
    
    dev_data->memory = device_memory_create(
        regions, 
        TEMP_REGION_COUNT, 
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
    struct device_rule_manager* rule_manager = temp_sensor_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_TEMP_SENSOR);
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取温度传感器寄存器
int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入温度传感器寄存器
int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 读取缓冲区
int temp_sensor_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length) {
    if (!instance || !buffer) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    // 简单实现，每次读取一个字节
    for (size_t i = 0; i < length; i++) {
        uint32_t value;
        if (temp_sensor_read(instance, addr + i, &value) != 0) {
            return -1;
        }
        buffer[i] = (uint8_t)value;
    }
    
    return 0;
}

// 写入缓冲区
int temp_sensor_write_buffer(device_instance_t* instance, uint32_t addr, const uint8_t* buffer, size_t length) {
    if (!instance || !buffer) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    // 简单实现，每次写入一个字节
    for (size_t i = 0; i < length; i++) {
        if (temp_sensor_write(instance, addr + i, buffer[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// 复位温度传感器
int temp_sensor_reset(device_instance_t* instance) {
    // 不执行任何操作，保持接口兼容性
    (void)instance;
    return 0;
}

// 销毁温度传感器
void temp_sensor_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
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

// 配置内存区域
int temp_sensor_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    // 重新配置内存区域
    device_memory_destroy(dev_data->memory);
    
    // 转换配置为内存区域
    memory_region_t* regions = (memory_region_t*)malloc(config_count * sizeof(memory_region_t));
    if (!regions) return -1;
    
    for (int i = 0; i < config_count; i++) {
        regions[i].base_addr = configs[i].base_addr;
        regions[i].unit_size = configs[i].unit_size;
        regions[i].length = configs[i].length;
        regions[i].data = NULL;
        regions[i].device_type = DEVICE_TYPE_TEMP_SENSOR;
        regions[i].device_id = instance->dev_id;
    }
    
    dev_data->memory = device_memory_create(
        regions, 
        config_count, 
        NULL, 
        DEVICE_TYPE_TEMP_SENSOR, 
        instance->dev_id
    );
    
    free(regions);
    return dev_data->memory ? 0 : -1;
}

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
        .get_mutex = temp_sensor_get_mutex,
        .get_rule_manager = temp_sensor_get_rule_manager,
        .configure_memory = temp_sensor_configure_memory
    };
    return &ops;
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
                        const action_target_array_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    
    // 使用通用的规则添加函数
    device_rule_manager_t manager;
    manager.rules = dev_data->device_rules;
    manager.rule_count = dev_data->rule_count;
    manager.mutex = &dev_data->mutex;
    
    int result = device_rule_add(&manager, addr, expected_value, expected_mask, targets);
    
    // 更新规则计数
    dev_data->rule_count = manager.rule_count;
    
    return result;
} 