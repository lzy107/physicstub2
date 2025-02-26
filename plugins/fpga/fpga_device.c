#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fpga_device.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"

// 注册FPGA设备
REGISTER_DEVICE(DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops);

// FPGA中断回调函数
static void fpga_irq_callback(void* data) {
    if (data) {
        printf("FPGA interrupt triggered! IRQ status: 0x%X\n", *(uint32_t*)data);
    }
}

// FPGA设备规则表
static rule_table_entry_t fpga_rules[1];

// 初始化FPGA规则表
static void init_fpga_rules(void) {
    // 创建目标处理动作
    action_target_t* target = action_target_create(
        ACTION_TYPE_CALLBACK,
        DEVICE_TYPE_FPGA,
        0,
        0,
        0,
        0,
        fpga_irq_callback,
        NULL
    );
    
    // 创建规则触发条件
    rule_trigger_t trigger = rule_trigger_create(FPGA_IRQ_REG, 1, 0xFF);
    
    // 设置规则
    fpga_rules[0].name = "FPGA IRQ Handler";
    fpga_rules[0].trigger = trigger;
    fpga_rules[0].targets = target;
    fpga_rules[0].priority = 2;
}

// 获取FPGA规则表
static const rule_table_entry_t* get_fpga_rules(void) {
    return fpga_rules;
}

// 获取FPGA规则数量
static int get_fpga_rule_count(void) {
    return sizeof(fpga_rules) / sizeof(fpga_rules[0]);
}

// 规则提供者
static rule_provider_t fpga_rule_provider = {
    .provider_name = "FPGA Rules",
    .get_rules = get_fpga_rules,
    .get_rule_count = get_fpga_rule_count
};

// 私有函数声明
static int fpga_init(device_instance_t* instance);
static int fpga_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int fpga_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void fpga_reset(device_instance_t* instance);
static void fpga_destroy(device_instance_t* instance);

// 获取FPGA设备互斥锁
static pthread_mutex_t* fpga_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    return &dev_data->mutex;
}

// FPGA设备操作接口
static device_ops_t fpga_ops = {
    .init = fpga_init,
    .read = fpga_read,
    .write = fpga_write,
    .reset = fpga_reset,
    .destroy = fpga_destroy,
    .get_mutex = fpga_get_mutex
};

// 获取FPGA设备操作接口
device_ops_t* get_fpga_device_ops(void) {
    return &fpga_ops;
}

// FPGA工作线程函数
static void* fpga_worker_thread(void* arg) {
    device_instance_t* instance = (device_instance_t*)arg;
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    uint32_t value;
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 读取配置寄存器
        if (device_memory_read(dev_data->memory, FPGA_CONFIG_REG, &value) == 0) {
            // 检查FPGA是否使能
            if (value & CONFIG_ENABLE) {
                // 读取控制寄存器
                if (device_memory_read(dev_data->memory, FPGA_CONTROL_REG, &value) == 0) {
                    // 检查是否有操作请求
                    if (value & CTRL_START) {
                        // 设置忙状态
                        uint32_t status = STATUS_BUSY;
                        device_memory_write(dev_data->memory, FPGA_STATUS_REG, status);
                        
                        // 模拟FPGA操作
                        usleep(100000);  // 模拟操作耗时100ms
                        
                        // 清除忙状态，设置完成状态
                        status = STATUS_DONE;
                        device_memory_write(dev_data->memory, FPGA_STATUS_REG, status);
                        
                        // 如果中断使能，触发中断
                        if (device_memory_read(dev_data->memory, FPGA_CONFIG_REG, &value) == 0) {
                            if (value & CONFIG_IRQ_EN) {
                                uint32_t irq = 0x1;
                                device_memory_write(dev_data->memory, FPGA_IRQ_REG, irq);
                            }
                        }
                        
                        // 清除启动位
                        value &= ~CTRL_START;
                        device_memory_write(dev_data->memory, FPGA_CONTROL_REG, value);
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&dev_data->mutex);
        usleep(10000);  // 10ms轮询间隔
    }
    
    return NULL;
}

// 初始化FPGA设备
static int fpga_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 初始化规则表
    init_fpga_rules();
    
    // 注册规则提供者
    action_manager_register_provider(&fpga_rule_provider);
    
    // 分配设备私有数据
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)calloc(1, sizeof(fpga_dev_data_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->running = 1;
    
    // 创建设备内存
    dev_data->memory = device_memory_create(FPGA_DATA_START + 0x2000, NULL, DEVICE_TYPE_FPGA, instance->dev_id);
    if (!dev_data->memory) {
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 设置初始状态
    uint32_t status = STATUS_READY;
    device_memory_write(dev_data->memory, FPGA_STATUS_REG, status);
    
    // 启动工作线程
    if (pthread_create(&dev_data->worker_thread, NULL, fpga_worker_thread, instance) != 0) {
        device_memory_destroy(dev_data->memory);
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FPGA寄存器或数据
static int fpga_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    int ret = device_memory_read(dev_data->memory, addr, value);
    pthread_mutex_unlock(&dev_data->mutex);
    
    return ret;
}

// 写入FPGA寄存器或数据
static int fpga_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = 0;
    if (addr == FPGA_CONFIG_REG) {
        // 如果设置了复位位，执行复位
        if (value & CONFIG_RESET) {
            fpga_reset(instance);
            value &= ~CONFIG_RESET;  // 清除复位位
        }
    }
    
    ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FPGA设备
static void fpga_reset(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 复位所有寄存器
    uint32_t zero = 0;
    device_memory_write(dev_data->memory, FPGA_CONFIG_REG, zero);
    device_memory_write(dev_data->memory, FPGA_CONTROL_REG, zero);
    device_memory_write(dev_data->memory, FPGA_IRQ_REG, zero);
    
    uint32_t status = STATUS_READY;
    device_memory_write(dev_data->memory, FPGA_STATUS_REG, status);
    
    // 清除数据区
    memset(dev_data->memory->data + FPGA_DATA_START, 0, 0x1000);
}

// 销毁FPGA设备
static void fpga_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 停止工作线程
    dev_data->running = 0;
    pthread_join(dev_data->worker_thread, NULL);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_memory_destroy(dev_data->memory);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}

// 注册FPGA设备类型
void register_fpga_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_fpga_device_ops();
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", ops);
} 