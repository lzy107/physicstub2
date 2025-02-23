#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "temp_sensor.h"

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
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 如果不在关断模式
        if (!(dev_data->config_reg & CONFIG_SHUTDOWN)) {
            // 模拟温度变化（在20-30度之间随机波动）
            int temp = 2000 + (rand() % 1000);  // 0.0625°C/bit
            dev_data->temp_reg = temp;
            
            // 检查报警条件
            if (dev_data->config_reg & CONFIG_ALERT) {
                if (temp >= dev_data->thigh_reg) {
                    dev_data->alert_status = 1;
                } else if (temp <= dev_data->tlow_reg) {
                    dev_data->alert_status = 1;
                } else {
                    dev_data->alert_status = 0;
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
    
    // 初始化互斥锁和数据
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->temp_reg = 2500;    // 25°C
    dev_data->config_reg = 0;     // 正常模式
    dev_data->tlow_reg = 1800;    // 18°C
    dev_data->thigh_reg = 3000;   // 30°C
    dev_data->alert_status = 0;
    dev_data->running = 1;
    
    // 启动温度更新线程
    if (pthread_create(&dev_data->update_thread, NULL, temp_update_thread, instance) != 0) {
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
    
    switch (addr) {
        case TEMP_REG:
            *value = dev_data->temp_reg;
            break;
        case CONFIG_REG:
            *value = dev_data->config_reg;
            break;
        case TLOW_REG:
            *value = dev_data->tlow_reg;
            break;
        case THIGH_REG:
            *value = dev_data->thigh_reg;
            break;
        default:
            pthread_mutex_unlock(&dev_data->mutex);
            return -1;
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return 0;
}

// 写入温度传感器寄存器
static int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    switch (addr) {
        case CONFIG_REG:
            dev_data->config_reg = value & 0xFF;
            break;
        case TLOW_REG:
            dev_data->tlow_reg = value & 0xFFFF;
            break;
        case THIGH_REG:
            dev_data->thigh_reg = value & 0xFFFF;
            break;
        default:
            pthread_mutex_unlock(&dev_data->mutex);
            return -1;
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return 0;
}

// 复位温度传感器
static void temp_sensor_reset(device_instance_t* instance) {
    if (!instance) return;
    
    temp_sensor_data_t* dev_data = (temp_sensor_data_t*)instance->private_data;
    if (!dev_data) return;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    dev_data->temp_reg = 2500;    // 25°C
    dev_data->config_reg = 0;     // 正常模式
    dev_data->tlow_reg = 1800;    // 18°C
    dev_data->thigh_reg = 3000;   // 30°C
    dev_data->alert_status = 0;
    
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
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
} 