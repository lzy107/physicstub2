#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fpga_device.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/device_rules.h"
#include "../include/device_rule_configs.h"

// 定义FPGA内存区域常量
#define FPGA_CONFIG_START 0x100
#define FPGA_MEM_SIZE     0x10000

// 注册FPGA设备
REGISTER_DEVICE(DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops);

// FPGA中断回调函数
void fpga_irq_callback(void* context, uint32_t addr, uint32_t value) {
    (void)addr; // 未使用参数
    if (context) {
        printf("FPGA interrupt triggered! IRQ status: 0x%X\n", value);
    }
}

// FPGA控制回调函数
void fpga_control_callback(void* context, uint32_t addr, uint32_t value) {
    (void)context; // 未使用参数
    (void)addr;    // 未使用参数
    printf("FPGA control register changed: 0x%08x\n", value);
}

// FPGA配置回调函数
void fpga_config_callback(void* context, uint32_t addr, uint32_t value) {
    (void)context; // 未使用参数
    (void)addr;    // 未使用参数
    printf("FPGA configuration register changed: 0x%08x\n", value);
}

// FPGA 设备内存区域全局配置
static const memory_region_t fpga_memory_regions[] = {
    // 寄存器区域
    {
        .base_addr = 0x00,
        .unit_size = 4,  // 4字节单位
        .length = 16,    // 16个寄存器
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0   // 默认ID为0，实际使用时会被覆盖
    },
    // 配置区域
    {
        .base_addr = FPGA_CONFIG_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_DATA_START - FPGA_CONFIG_START) / 4,
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0   // 默认ID为0，实际使用时会被覆盖
    },
    // 数据区域
    {
        .base_addr = FPGA_DATA_START,
        .unit_size = 4,  // 4字节单位
        .length = (FPGA_MEM_SIZE - FPGA_DATA_START) / 4,
        .data = NULL,    // 初始化时分配
        .device_type = DEVICE_TYPE_FPGA,
        .device_id = 0   // 默认ID为0，实际使用时会被覆盖
    }
};

#define FPGA_REGION_COUNT (sizeof(fpga_memory_regions) / sizeof(fpga_memory_regions[0]))


// 获取FPGA设备互斥锁
static pthread_mutex_t* fpga_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    return &dev_data->mutex;
}

// 获取FPGA设备规则管理器
struct device_rule_manager* fpga_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->private_data) return NULL;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    
    // 初始化规则管理器
    dev_data->rule_manager.rules = dev_data->device_rules;
    dev_data->rule_manager.rule_count = dev_data->rule_count;
    
    return &dev_data->rule_manager;
}

// 获取FPGA设备操作接口
device_ops_t* get_fpga_device_ops(void) {
    static device_ops_t ops = {
        .init = fpga_device_init,
        .destroy = fpga_device_destroy,
        .read = fpga_device_read,
        .write = fpga_device_write,
        .read_buffer = fpga_device_read_buffer,
        .write_buffer = fpga_device_write_buffer,
        .reset = fpga_device_reset,
        .get_mutex = fpga_get_mutex,
        .get_rule_manager = fpga_get_rule_manager,
        .configure_memory = fpga_configure_memory
    };
    return &ops;
}

// 初始化FPGA设备
int fpga_device_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    fpga_device_t* dev_data = (fpga_device_t*)calloc(1, sizeof(fpga_device_t));
    if (!dev_data) return -1;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 创建设备内存
    int region_count = 3; // FPGA有3个内存区域
    memory_region_t regions[3];
    
    // 复制内存区域配置
    memcpy(regions, fpga_memory_regions, sizeof(regions));
    
    // 更新设备ID
    for (int i = 0; i < region_count; i++) {
        regions[i].device_id = instance->dev_id;
    }
    
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
    struct device_rule_manager* rule_manager = fpga_get_rule_manager(instance);
    if (rule_manager) {
        // 使用全局规则配置设置规则
        dev_data->rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_FPGA);
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FPGA寄存器或数据
int fpga_device_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FPGA寄存器或数据
int fpga_device_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接写入内存
    int ret = device_memory_write(dev_data->memory, addr, value);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 读取缓冲区
int fpga_device_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length) {
    if (!instance || !buffer) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    // 简单实现，每次读取一个字节
    for (size_t i = 0; i < length; i++) {
        uint32_t value;
        if (fpga_device_read(instance, addr + i, &value) != 0) {
            return -1;
        }
        buffer[i] = (uint8_t)value;
    }
    
    return 0;
}

// 写入缓冲区
int fpga_device_write_buffer(device_instance_t* instance, uint32_t addr, const uint8_t* buffer, size_t length) {
    if (!instance || !buffer) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    // 简单实现，每次写入一个字节
    for (size_t i = 0; i < length; i++) {
        if (fpga_device_write(instance, addr + i, buffer[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// 复位FPGA设备
int fpga_device_reset(device_instance_t* instance) {
    if (!instance) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data || !dev_data->memory) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 写入复位命令到配置寄存器
    uint32_t config = CONFIG_RESET;
    int ret = device_memory_write(dev_data->memory, FPGA_CONFIG_REG, config);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 销毁FPGA设备
void fpga_device_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    if (!dev_data) return;
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放设备内存
    device_memory_destroy(dev_data->memory);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
}

// 配置内存区域
int fpga_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) return -1;
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
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
        regions[i].device_type = DEVICE_TYPE_FPGA;
        regions[i].device_id = instance->dev_id;
    }
    
    dev_data->memory = device_memory_create(
        regions, 
        config_count, 
        NULL, 
        DEVICE_TYPE_FPGA, 
        instance->dev_id
    );
    
    free(regions);
    return dev_data->memory ? 0 : -1;
}

// 向FPGA设备添加规则
int fpga_add_rule(device_instance_t* instance, uint32_t addr, 
                 uint32_t expected_value, uint32_t expected_mask, 
                 const action_target_array_t* targets) {
    if (!instance || !instance->private_data || !targets) {
        return -1;
    }
    
    fpga_device_t* dev_data = (fpga_device_t*)instance->private_data;
    
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

// 注册FPGA设备类型
void register_fpga_device_type(device_manager_t* dm) {
    if (!dm) return;
    
    device_ops_t* ops = get_fpga_device_ops();
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", ops);
} 