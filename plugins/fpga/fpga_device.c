#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fpga_device.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/device_rules.h"
#include "../include/device_rule_configs.h"

// 注册FPGA设备
REGISTER_DEVICE(DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops);

// FPGA中断回调函数
static void fpga_irq_callback(void* data) {
    if (data) {
        printf("FPGA interrupt triggered! IRQ status: 0x%X\n", *(uint32_t*)data);
    }
}

// FPGA 设备内存区域全局配置
static const memory_region_t fpga_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 16,    // 16个寄存器
        .data = NULL     // 初始化时分配
    },
    // 配置区域
    {
        .base_addr = FPGA_CONFIG_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_DATA_START - FPGA_CONFIG_START) / 4,
        .data = NULL     // 初始化时分配
    },
    // 数据区域
    {
        .base_addr = FPGA_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_MEM_SIZE - FPGA_DATA_START) / 4,
        .data = NULL     // 初始化时分配
    }
};

#define FPGA_REGION_COUNT (sizeof(fpga_memory_regions) / sizeof(fpga_memory_regions[0]))

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

// 获取FPGA设备规则管理器
static device_rule_manager_t* fpga_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    
    // 使用静态变量存储规则管理器，避免每次调用都分配内存
    static device_rule_manager_t rule_manager;
    
    // 初始化规则管理器
    rule_manager.rules = dev_data->device_rules;
    rule_manager.rule_count = dev_data->rule_count;
    rule_manager.max_rules = 8; // FPGA设备支持8个规则
    rule_manager.mutex = &dev_data->mutex;
    
    return &rule_manager;
}

// FPGA设备操作接口
static device_ops_t fpga_ops = {
    .init = fpga_init,
    .read = fpga_read,
    .write = fpga_write,
    .reset = fpga_reset,
    .destroy = fpga_destroy,
    .get_mutex = fpga_get_mutex,
    .get_rule_manager = fpga_get_rule_manager
};

// 获取FPGA设备操作接口
device_ops_t* get_fpga_device_ops(void) {
    static device_ops_t ops = {
        .init = fpga_init,
        .destroy = fpga_destroy,
        .read = fpga_read,
        .write = fpga_write,
        .reset = fpga_reset,
        .get_rule_manager = fpga_get_rule_manager,
        .configure_memory = fpga_configure_memory
    };
    return &ops;
}

// 初始化FPGA设备
static int fpga_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)calloc(1, sizeof(fpga_dev_data_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 获取设备内存配置
    int region_count;
    const memory_region_t* regions = get_device_memory_regions(DEVICE_TYPE_FPGA, &region_count);
    
    // 创建设备内存
    dev_data->memory = device_memory_create(
        regions, 
        region_count, 
        NULL, 
        DEVICE_TYPE_FPGA, 
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
    device_rule_manager_t* rule_manager = fpga_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_FPGA);
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FPGA寄存器或数据
static int fpga_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FPGA寄存器或数据
static int fpga_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FPGA设备 - 简化为空函数
static void fpga_reset(device_instance_t* instance) {
    // 不执行任何操作，保持接口兼容性
    (void)instance;
}

// 销毁FPGA设备
static void fpga_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 不再需要停止工作线程
    
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

// 向FPGA设备添加规则
int fpga_add_rule(device_instance_t* instance, uint32_t addr, 
                 uint32_t expected_value, uint32_t expected_mask, 
                 action_target_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    
    // 使用通用的规则添加函数
    device_rule_manager_t manager;
    device_rule_manager_init(&manager, &dev_data->mutex, dev_data->device_rules, 8);
    manager.rule_count = dev_data->rule_count;
    
    int result = device_rule_add(&manager, addr, expected_value, expected_mask, targets);
    
    // 更新规则计数
    dev_data->rule_count = manager.rule_count;
    
    return result;
} 