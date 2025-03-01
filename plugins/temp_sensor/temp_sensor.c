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
            .length = 16,  // 增加长度以支持更多寄存器
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
    
    // 初始化温度值（25°C）
    uint32_t initial_temp = 2500;  // 25°C (0.0625°C/bit)
    
    // 在设备内存中设置初始值
    device_memory_write(dev_data->memory, TEMP_REG, initial_temp);
    device_memory_write(dev_data->memory, CONFIG_REG, CONFIG_ALERT);
    device_memory_write(dev_data->memory, TLOW_REG, 1000);  // 10°C
    device_memory_write(dev_data->memory, THIGH_REG, 3000); // 30°C
    device_memory_write(dev_data->memory, 0x10, initial_temp); // 初始模拟温度也是25°C
    
    printf("DEBUG: 温度传感器设备内存初始化完成，ID: %d\n", instance->dev_id);
    
    // 设置设备私有数据
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
    
    // 调试输出
    printf("DEBUG: 温度传感器读取寄存器 0x%08X, 值 = 0x%08X, 返回值 = %d\n", 
           addr, *value, ret);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入温度传感器寄存器
int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 写入设备内存
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
    if (!instance) return -1;
    
    // 复位寄存器到默认值
    temp_sensor_write(instance, CONFIG_REG, 0);
    
    return 0;
}

// 销毁温度传感器
void temp_sensor_destroy(device_instance_t* instance) {
    if (!instance || !instance->private_data) return;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    
    // 销毁内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
    }
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放私有数据
    free(dev_data);
    instance->private_data = NULL;
}

// 配置内存区域
int temp_sensor_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    // 销毁现有内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存区域
    memory_region_t* regions = (memory_region_t*)malloc(config_count * sizeof(memory_region_t));
    if (!regions) return -1;
    
    // 转换配置到内存区域
    for (int i = 0; i < config_count; i++) {
        regions[i].base_addr = configs[i].base_addr;
        regions[i].unit_size = configs[i].unit_size;
        regions[i].length = configs[i].length;
        regions[i].data = NULL;
        regions[i].device_type = DEVICE_TYPE_TEMP_SENSOR;
        regions[i].device_id = instance->dev_id;
    }
    
    // 创建新的设备内存
    dev_data->memory = device_memory_create(
        regions, 
        config_count, 
        NULL, 
        DEVICE_TYPE_TEMP_SENSOR, 
        instance->dev_id
    );
    
    free(regions);
    
    return (dev_data->memory) ? 0 : -1;
}

// 获取温度传感器操作接口
device_ops_t* get_temp_sensor_ops(void) {
    static device_ops_t ops = {
        .init = temp_sensor_init,
        .read = temp_sensor_read,
        .write = temp_sensor_write,
        .read_buffer = temp_sensor_read_buffer,
        .write_buffer = temp_sensor_write_buffer,
        .reset = temp_sensor_reset,
        .destroy = temp_sensor_destroy,
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
    if (!instance || !instance->private_data || !targets) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->private_data;
    
    // 检查规则数量是否已达上限
    if (dev_data->rule_count >= 8) {
        printf("温度传感器规则数量已达上限\n");
        return -1;
    }
    
    // 添加新规则
    device_rule_t* rule = &dev_data->device_rules[dev_data->rule_count];
    rule->addr = addr;
    rule->expected_value = expected_value;
    rule->expected_mask = expected_mask;
    
    // 复制目标数组
    memcpy(&rule->targets, targets, sizeof(action_target_array_t));
    
    dev_data->rule_count++;
    
    return 0;
}