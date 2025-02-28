// flash_device.c
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "flash_device.h"
#include "../include/device_registry.h"
#include "../include/device_memory.h"
#include "../include/action_manager.h"
#include "../include/device_rules.h"
#include "../include/device_configs.h"
#include "../include/device_rule_configs.h"

// 注册FLASH设备
REGISTER_DEVICE(DEVICE_TYPE_FLASH, "FLASH", get_flash_device_ops);

// 私有函数声明
static int flash_init(device_instance_t* instance);
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void flash_reset(device_instance_t* instance);
static void flash_destroy(device_instance_t* instance);
static pthread_mutex_t* flash_get_mutex(device_instance_t* instance);
static device_rule_manager_t* flash_get_rule_manager(device_instance_t* instance);

// FLASH设备操作接口实现
static device_ops_t flash_ops = {
    .init = flash_init,
    .read = flash_read,
    .write = flash_write,
    .reset = flash_reset,
    .destroy = flash_destroy,
    .get_mutex = flash_get_mutex,
    .get_rule_manager = flash_get_rule_manager
};

// 获取FLASH设备操作接口
device_ops_t* get_flash_device_ops(void) {
    return &flash_ops;
}

// 初始化FLASH设备
static int flash_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    flash_device_t* dev_data = (flash_device_t*)calloc(1, sizeof(flash_device_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 获取设备内存配置
    int region_count;
    const memory_region_t* regions = get_device_memory_regions(DEVICE_TYPE_FLASH, &region_count);
    
    // 创建设备内存
    dev_data->memory = device_memory_create(
        regions, 
        region_count, 
        NULL, 
        DEVICE_TYPE_FLASH, 
        instance->dev_id
    );
    
    if (!dev_data->memory) {
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 初始化所有数据区域为0xFF（FLASH擦除状态）
    for (int i = 0; i < dev_data->memory->region_count; i++) {
        memory_region_t* region = &dev_data->memory->regions[i];
        // 检查是否是数据区域（基地址大于等于FLASH_DATA_START）
        if (region->base_addr >= FLASH_DATA_START) {
            // 只初始化数据区域
            memset(region->data, 0xFF, region->unit_size * region->length);
        }
    }
    
    // 初始化设备状态
    dev_data->status = FLASH_STATUS_READY;
    dev_data->control = 0;
    dev_data->config = 0;
    dev_data->address = 0;
    dev_data->size = FLASH_MEM_SIZE;
    
    // 初始化规则计数器
    dev_data->rule_count = 0;
    
    // 初始化设备规则
    device_rule_manager_t* rule_manager = flash_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_FLASH);
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FLASH数据或寄存器
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = -1;
    
    // 读取寄存器
    if (addr == FLASH_REG_STATUS) {
        *value = dev_data->status;
        ret = 0;
    }
    else if (addr == FLASH_REG_CONTROL) {
        *value = dev_data->control;
        ret = 0;
    }
    else if (addr == FLASH_REG_CONFIG) {
        *value = dev_data->config;
        ret = 0;
    }
    else if (addr == FLASH_REG_ADDRESS) {
        *value = dev_data->address;
        ret = 0;
    }
    else if (addr == FLASH_REG_SIZE) {
        *value = dev_data->size;
        ret = 0;
    }
    else if (addr == FLASH_REG_DATA) {
        // 读取当前地址的数据
        uint32_t data_addr = dev_data->address;
        if (data_addr < FLASH_MEM_SIZE) {
            ret = device_memory_read(dev_data->memory, data_addr, value);
        }
    }
    else {
        // 直接读取内存
        ret = device_memory_read(dev_data->memory, addr, value);
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FLASH数据或寄存器
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = -1;
    
    // 写入寄存器
    if (addr == FLASH_REG_STATUS) {
        // 状态寄存器写保护检查
        if (!(dev_data->status & FLASH_STATUS_SRWD)) {
            dev_data->status = value & 0xFF;
            ret = 0;
        }
    }
    else if (addr == FLASH_REG_CONTROL) {
        dev_data->control = value & 0xFF;
        ret = 0;
        
        // 处理控制命令
        if (value == FLASH_CTRL_ERASE) {
            // 擦除操作 - 将数据区域填充为0xFF
            memset(dev_data->memory->regions[1].data, 0xFF, dev_data->memory->regions[1].unit_size * dev_data->memory->regions[1].length);
            dev_data->status |= FLASH_STATUS_WEL; // 设置写使能
        }
    }
    else if (addr == FLASH_REG_CONFIG) {
        dev_data->config = value & 0xFF;
        ret = 0;
    }
    else if (addr == FLASH_REG_ADDRESS) {
        dev_data->address = value;
        ret = 0;
    }
    else if (addr == FLASH_REG_DATA) {
        // 写入当前地址的数据
        uint32_t data_addr = dev_data->address;
        if (data_addr < FLASH_MEM_SIZE) {
            // 检查写使能
            if (dev_data->status & FLASH_STATUS_WEL) {
                // 模拟FLASH只能将1改为0的特性
                uint32_t old_value;
                if (device_memory_read(dev_data->memory, data_addr, &old_value) == 0) {
                    value &= old_value;  // 只能将1改为0
                    ret = device_memory_write(dev_data->memory, data_addr, value);
                    
                    // 自动增加地址
                    dev_data->address += 4;
                    
                    // 清除写使能
                    dev_data->status &= ~FLASH_STATUS_WEL;
                }
            }
        }
    }
    else {
        // 直接写入内存
        // 检查写使能
        if (dev_data->status & FLASH_STATUS_WEL) {
            ret = device_memory_write(dev_data->memory, addr, value);
            
            // 清除写使能
            dev_data->status &= ~FLASH_STATUS_WEL;
        }
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FLASH设备
static void flash_reset(device_instance_t* instance) {
    if (!instance) return;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 清除所有寄存器
    dev_data->status = FLASH_STATUS_READY;
    dev_data->control = 0;
    dev_data->config = 0;
    dev_data->address = 0;
    
    // 将所有数据恢复为0xFF（FLASH擦除状态）
    memset(dev_data->memory->regions[1].data, 0xFF, dev_data->memory->regions[1].unit_size * dev_data->memory->regions[1].length);
    
    pthread_mutex_unlock(&dev_data->mutex);
}

// 销毁FLASH设备
static void flash_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return;
    
    // 清理设备规则
    for (int i = 0; i < dev_data->rule_count; i++) {
        if (dev_data->device_rules[i].targets) {
            action_target_destroy(dev_data->device_rules[i].targets);
            dev_data->device_rules[i].targets = NULL;
        }
    }
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_memory_destroy(dev_data->memory);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}

// 获取Flash设备互斥锁
static pthread_mutex_t* flash_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    return &dev_data->mutex;
}

// 获取Flash设备规则管理器
static device_rule_manager_t* flash_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    
    // 使用静态变量存储规则管理器，避免每次调用都分配内存
    static device_rule_manager_t rule_manager;
    
    // 初始化规则管理器
    rule_manager.rules = dev_data->device_rules;
    rule_manager.rule_count = dev_data->rule_count;
    rule_manager.max_rules = 8; // Flash设备支持8个规则
    rule_manager.mutex = &dev_data->mutex;
    
    return &rule_manager;
}

// 注册FLASH设备类型
void register_flash_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_flash_device_ops();
    device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", ops);
}

// 向Flash设备添加规则
int flash_device_add_rule(device_instance_t* instance, uint32_t addr, 
                         uint32_t expected_value, uint32_t expected_mask, 
                         action_target_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    
    // 使用通用的规则添加函数
    device_rule_manager_t manager;
    device_rule_manager_init(&manager, &dev_data->mutex, dev_data->device_rules, 8);
    manager.rule_count = dev_data->rule_count;
    
    int result = device_rule_add(&manager, addr, expected_value, expected_mask, targets);
    
    // 更新规则计数
    dev_data->rule_count = manager.rule_count;
    
    return result;
}

// 配置Flash设备内存区域
int flash_configure_memory_regions(flash_device_t* dev_data, memory_region_config_t* configs, int config_count) {
    if (!dev_data || !configs || config_count <= 0) {
        return -1;
    }
    
    // 销毁现有的内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存
    dev_data->memory = device_memory_create_from_config(configs, config_count, NULL, DEVICE_TYPE_FLASH, 0);
    if (!dev_data->memory) {
        return -1;
    }
    
    // 初始化所有数据区域为0xFF（FLASH擦除状态）
    for (int i = 0; i < dev_data->memory->region_count; i++) {
        memory_region_t* region = &dev_data->memory->regions[i];
        // 检查是否是数据区域（基地址大于等于FLASH_DATA_START）
        if (region->base_addr >= FLASH_DATA_START) {
            // 只初始化数据区域
            memset(region->data, 0xFF, region->unit_size * region->length);
        }
    }
    
    return 0;
}