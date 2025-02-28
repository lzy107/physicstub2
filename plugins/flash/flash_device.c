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
static int flash_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count);

// FLASH设备操作接口实现
static device_ops_t flash_ops = {
    .init = flash_init,
    .read = flash_read,
    .write = flash_write,
    .reset = flash_reset,
    .destroy = flash_destroy,
    .get_mutex = flash_get_mutex,
    .get_rule_manager = flash_get_rule_manager,
    .configure_memory = flash_configure_memory
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

// 读取FLASH寄存器或数据
static int flash_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FLASH寄存器或数据
static int flash_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FLASH设备
static void flash_reset(device_instance_t* instance) {
    // 不执行任何操作，保持接口兼容性
    (void)instance;
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

// 配置FLASH设备内存
static int flash_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) {
        return -1;
    }
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) {
        return -1;
    }
    
    // 销毁现有的内存
    if (dev_data->memory) {
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 创建新的内存
    dev_data->memory = device_memory_create_from_config(configs, config_count, NULL, DEVICE_TYPE_FLASH, instance->dev_id);
    if (!dev_data->memory) {
        return -1;
    }
    
    // 初始化寄存器
    device_memory_write(dev_data->memory, FLASH_REG_STATUS, STATUS_READY);
    device_memory_write(dev_data->memory, FLASH_REG_CONFIG, 0);
    device_memory_write(dev_data->memory, FLASH_REG_ADDR, 0);
    device_memory_write(dev_data->memory, FLASH_REG_DATA, 0);
    device_memory_write(dev_data->memory, FLASH_REG_SIZE, FLASH_MEM_SIZE);
    
    return 0;
}