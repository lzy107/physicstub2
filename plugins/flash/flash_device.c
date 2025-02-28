// flash_device.c
#include <stdio.h>
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
static int flash_reset(device_instance_t* instance);
static void flash_destroy(device_instance_t* instance);
static pthread_mutex_t* flash_get_mutex(device_instance_t* instance);
static struct device_rule_manager* flash_get_rule_manager(device_instance_t* instance);
static int flash_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count);

// FLASH设备操作接口实现
static device_ops_t flash_ops = {
    .init = flash_init,
    .read = flash_read,
    .write = flash_write,
    .reset = flash_reset,
    .destroy = flash_destroy,
    .get_mutex = flash_get_mutex,
    .get_rule_manager = (struct device_rule_manager* (*)(device_instance_t*))flash_get_rule_manager,
    .configure_memory = flash_configure_memory
};

// 获取FLASH设备操作接口
device_ops_t* get_flash_device_ops(void) {
    return &flash_ops;
}

// 初始化FLASH设备
static int flash_init(device_instance_t* instance) {
    printf("开始初始化Flash设备...\n");
    if (!instance) {
        printf("Flash设备初始化失败：实例为空\n");
        return -1;
    }
    
    // 分配设备私有数据
    flash_device_t* dev_data = (flash_device_t*)calloc(1, sizeof(flash_device_t));
    if (!dev_data) {
        printf("Flash设备初始化失败：内存分配失败\n");
        return -1;
    }
    printf("Flash设备私有数据分配成功\n");
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    printf("Flash设备互斥锁初始化成功\n");
    
    // 获取设备内存配置
    printf("获取Flash设备内存配置...\n");
    int region_count;
    const memory_region_t* regions = get_device_memory_regions(DEVICE_TYPE_FLASH, &region_count);
    printf("获取到 %d 个内存区域\n", region_count);
    
    // 创建设备内存
    printf("创建Flash设备内存...\n");
    dev_data->memory = device_memory_create(
        regions, 
        region_count, 
        NULL, 
        DEVICE_TYPE_FLASH, 
        instance->dev_id
    );
    
    if (!dev_data->memory) {
        printf("Flash设备内存创建失败\n");
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    printf("Flash设备内存创建成功\n");
    
    // 初始化规则计数器
    dev_data->rule_count = 0;
    
    // 初始化设备规则
    printf("初始化Flash设备规则...\n");
    struct device_rule_manager* rule_manager = flash_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        printf("使用全局规则配置设置Flash设备规则...\n");
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_FLASH);
        printf("设置了 %d 条Flash设备规则\n", dev_data->rule_count);
    } else {
        printf("获取Flash设备规则管理器失败\n");
    }
    
    instance->private_data = dev_data;
    printf("Flash设备初始化完成\n");
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
    
    // 处理特殊寄存器
    if (addr == FLASH_REG_CONTROL) {
        // 执行控制命令
        if (value == FLASH_CTRL_WRITE || value == FLASH_CTRL_READ || value == FLASH_CTRL_ERASE) {
            // 读取当前状态寄存器的值
            uint32_t current_status;
            device_memory_read(dev_data->memory, FLASH_REG_STATUS, &current_status);
            
            // 保留写使能位，同时设置就绪状态
            uint32_t new_status = (current_status & FLASH_STATUS_WEL) | FLASH_STATUS_READY;
            
            // 更新状态寄存器，设置为就绪状态，同时保留写使能位
            device_memory_write(dev_data->memory, FLASH_REG_STATUS, new_status);
            printf("DEBUG: Flash设备执行命令: 0x%02X, 更新状态寄存器为就绪状态，保留写使能位\n", value);
            
            // 更新地址空间中的状态寄存器值
            address_space_t* as = instance->addr_space;
            if (as) {
                address_space_write(as, FLASH_REG_STATUS, new_status);
                printf("DEBUG: 同步Flash设备地址空间状态寄存器，值为: 0x%02X\n", new_status);
            }
        }
    }
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FLASH设备
static int flash_reset(device_instance_t* instance) {
    // 不执行任何操作，保持接口兼容性
    (void)instance;
    return 0;
}

// 销毁FLASH设备
static void flash_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    if (!dev_data) return;
    
    // 清理设备规则
    for (int i = 0; i < dev_data->rule_count; i++) {
        if (dev_data->device_rules[i].targets) {
            free(dev_data->device_rules[i].targets);
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
static struct device_rule_manager* flash_get_rule_manager(device_instance_t* instance) {
    printf("获取Flash设备规则管理器...\n");
    if (!instance || !instance->private_data) {
        printf("Flash设备规则管理器获取失败：实例或私有数据为空\n");
        return NULL;
    }
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    printf("Flash设备私有数据获取成功\n");
    
    // 使用设备规则管理器初始化函数
    printf("初始化Flash设备规则管理器...\n");
    device_rule_manager_init(&dev_data->rule_manager, &dev_data->mutex);
    
    printf("Flash设备规则管理器初始化成功\n");
    return &dev_data->rule_manager;
}

// 注册FLASH设备类型
void register_flash_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_flash_device_ops();
    device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", ops);
}

// 向Flash设备添加规则
int flash_add_rule(device_instance_t* instance, uint32_t addr, 
                  uint32_t expected_value, uint32_t expected_mask, 
                  const action_target_array_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    flash_device_t* dev_data = (flash_device_t*)instance->private_data;
    
    // 使用通用的规则添加函数
    device_rule_manager_t manager;
    device_rule_manager_init(&manager, &dev_data->mutex);
    manager.rules = dev_data->device_rules;
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
    device_memory_write(dev_data->memory, FLASH_REG_STATUS, FLASH_STATUS_READY);
    device_memory_write(dev_data->memory, FLASH_REG_CONFIG, 0);
    device_memory_write(dev_data->memory, FLASH_REG_ADDRESS, 0);
    device_memory_write(dev_data->memory, FLASH_REG_DATA, 0);
    device_memory_write(dev_data->memory, FLASH_REG_SIZE, FLASH_MEM_SIZE);
    
    return 0;
}

// Flash 擦除回调函数
void flash_erase_callback(void* context, uint32_t addr, uint32_t value) {
    (void)addr; // 未使用参数
    if (context) {
        printf("Flash erase operation triggered! Control value: 0x%08x\n", value);
    }
}

// Flash 读取回调函数
void flash_read_callback(void* context, uint32_t addr, uint32_t value) {
    (void)addr; // 未使用参数
    if (context) {
        printf("Flash read operation triggered! Control value: 0x%08x\n", value);
    }
}

// Flash 写入回调函数
void flash_write_callback(void* context, uint32_t addr, uint32_t value) {
    (void)addr; // 未使用参数
    if (context) {
        printf("Flash write operation triggered! Control value: 0x%08x\n", value);
    }
}