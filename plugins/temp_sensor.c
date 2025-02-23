#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "temp_sensor.h"
#include "device_registry.h"
#include "action_manager.h"

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
static const rule_table_entry_t temp_sensor_rules[] = {
    {
        .name = "Temperature High Alert",
        .trigger_addr = TEMP_REG,
        .expect_value = 3000,  // 30°C
        .type = ACTION_TYPE_CALLBACK,
        .target_addr = 0,
        .action_value = 0,
        .priority = 1,
        .callback = temp_alert_callback
    }
};

// 获取温度传感器规则表
static const rule_table_entry_t* get_temp_sensor_rules(void) {
    return temp_sensor_rules;
}

// 获取温度传感器规则数量
static int get_temp_sensor_rule_count(void) {
    return sizeof(temp_sensor_rules) / sizeof(temp_sensor_rules[0]);
}

// 注册温度传感器规则提供者
REGISTER_RULE_PROVIDER(temp_sensor, get_temp_sensor_rules, get_temp_sensor_rule_count);

// 温度传感器内存区域配置
static device_mem_region_t temp_regions[TEMP_REGION_COUNT] = {
    // 寄存器区域（4个16位寄存器）
    {
        .start_addr = TEMP_REG,
        .unit_size = sizeof(uint16_t),
        .unit_count = 4,
        .mem_ptr = NULL
    }
};

// 私有函数声明
static int temp_sensor_init(device_instance_t* instance);
static int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void temp_sensor_reset(device_instance_t* instance);
static void temp_sensor_destroy(device_instance_t* instance);

// 温度传感器操作接口实现
static device_ops_t temp_sensor_ops = {
    .init = temp_sensor_init,
    .read = temp_sensor_read,
    .write = temp_sensor_write,
    .reset = temp_sensor_reset,
    .destroy = temp_sensor_destroy
};

// 获取温度传感器操作接口
device_ops_t* get_temp_sensor_ops(void) {
    return &temp_sensor_ops;
}

// 温度更新线程函数
static void* temp_update_thread(void* arg) {
    device_instance_t* instance = (device_instance_t*)arg;
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    uint16_t value;
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 读取配置寄存器
        if (device_mem_read(&dev_data->mem_config, CONFIG_REG, &value, sizeof(value)) == 0) {
            // 如果不在关断模式
            if (!(value & CONFIG_SHUTDOWN)) {
                // 模拟温度变化（在20-30度之间随机波动）
                int16_t temp = 2000 + (rand() % 1000);  // 0.0625°C/bit
                device_mem_write(&dev_data->mem_config, TEMP_REG, &temp, sizeof(temp));
                
                // 检查报警条件
                if (value & CONFIG_ALERT) {
                    int16_t tlow, thigh;
                    if (device_mem_read(&dev_data->mem_config, TLOW_REG, &tlow, sizeof(tlow)) == 0 &&
                        device_mem_read(&dev_data->mem_config, THIGH_REG, &thigh, sizeof(thigh)) == 0) {
                        // 更新报警状态
                        if (temp >= thigh || temp <= tlow) {
                            value |= (1 << 15);  // 设置报警位
                        } else {
                            value &= ~(1 << 15);  // 清除报警位
                        }
                        device_mem_write(&dev_data->mem_config, CONFIG_REG, &value, sizeof(value));
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
    
    // 初始化内存配置
    dev_data->mem_config.regions = temp_regions;
    dev_data->mem_config.region_count = TEMP_REGION_COUNT;
    
    // 初始化设备内存
    if (device_mem_init(&dev_data->mem_config) != 0) {
        free(dev_data);
        return -1;
    }
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->running = 1;
    
    // 设置初始值
    int16_t temp = 2500;    // 25°C
    int16_t tlow = 1800;    // 18°C
    int16_t thigh = 3000;   // 30°C
    uint16_t config = 0;     // 正常模式
    
    device_mem_write(&dev_data->mem_config, TEMP_REG, &temp, sizeof(temp));
    device_mem_write(&dev_data->mem_config, TLOW_REG, &tlow, sizeof(tlow));
    device_mem_write(&dev_data->mem_config, THIGH_REG, &thigh, sizeof(thigh));
    device_mem_write(&dev_data->mem_config, CONFIG_REG, &config, sizeof(config));
    
    // 启动温度更新线程
    if (pthread_create(&dev_data->update_thread, NULL, temp_update_thread, instance) != 0) {
        device_mem_destroy(&dev_data->mem_config);
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
    uint16_t reg_value;
    int ret = device_mem_read(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
    if (ret == 0) {
        *value = reg_value;
    }
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 写入温度传感器寄存器
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    uint16_t reg_value = (uint16_t)value;
    int ret = device_mem_write(&dev_data->mem_config, addr, &reg_value, sizeof(reg_value));
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 复位温度传感器
static void temp_sensor_reset(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 复位时不需要获取互斥锁，因为调用者已经持有锁
    
    // 复位所有寄存器到默认值
    int16_t temp = 2500;    // 25°C
    int16_t tlow = 1800;    // 18°C
    int16_t thigh = 3000;   // 30°C
    uint16_t config = 0;     // 正常模式
    
    device_mem_write(&dev_data->mem_config, TEMP_REG, &temp, sizeof(temp));
    device_mem_write(&dev_data->mem_config, TLOW_REG, &tlow, sizeof(tlow));
    device_mem_write(&dev_data->mem_config, THIGH_REG, &thigh, sizeof(thigh));
    device_mem_write(&dev_data->mem_config, CONFIG_REG, &config, sizeof(config));
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
    device_mem_destroy(&dev_data->mem_config);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
} 