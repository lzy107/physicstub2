#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "device_types.h"
#include "temp_sensor/temp_sensor.h"
#include "device_registry.h"
#include "action_manager.h"
#include "device_memory.h"
#include "device_rule_configs.h"

// 注册温度传感器设备
REGISTER_DEVICE(DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops);

// 温度报警回调函数
void temp_alert_callback(void* context, uint32_t addr, uint32_t value) {
    (void)context;
    (void)addr;
    if (context) {
        printf("Temperature alert triggered! Current temperature: %.2f°C\n", 
            value * 0.0625);
    }
}

// 配置回调函数
void temp_config_callback(void* context, uint32_t addr, uint32_t value) {
    (void)context;
    (void)addr;
    printf("Temperature sensor configuration changed: 0x%08x\n", value);
}

// 获取温度传感器互斥锁
static pthread_mutex_t* temp_sensor_get_mutex(device_instance_t* instance) {
    if (!instance || !instance->priv_data) {
        return NULL;
    }
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    return &dev_data->mutex;
}

// 获取温度传感器规则管理器
struct device_rule_manager* temp_sensor_get_rule_manager(device_instance_t* instance) {
    if (!instance || !instance->priv_data) {
        printf("获取规则管理器失败：无效的设备实例\n");
        return NULL;
    }
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    
    // 初始化规则管理器
    dev_data->rule_manager.mutex = &dev_data->mutex;
    dev_data->rule_manager.rules = dev_data->device_rules;
    dev_data->rule_manager.rule_count = dev_data->rule_count;
    dev_data->rule_manager.rule_capacity = MAX_DEVICE_RULES;
    
    printf("获取规则管理器：规则数量=%d，最大规则数=%d\n", 
           dev_data->rule_count, dev_data->rule_manager.rule_capacity);
    
    return &dev_data->rule_manager;
}

// 获取温度传感器设备内存
device_memory_t* temp_sensor_get_memory(device_instance_t* instance) {
    if (!instance || !instance->priv_data) {
        return NULL;
    }
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    return dev_data->memory;
}

// 初始化温度传感器
int temp_sensor_init(device_instance_t* instance) {
    if (!instance) {
        printf("温度传感器初始化失败：无效的设备实例\n");
        return -1;
    }

    // 分配设备私有数据
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)calloc(1, sizeof(temp_sensor_device_t));
    if (!dev_data) {
        printf("温度传感器初始化失败：无法分配内存\n");
        return -1;
    }
    
    // 初始化规则计数
    dev_data->rule_count = 0;
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    
    // 创建内存区域
    memory_region_t* regions = temp_sensor_memory_regions;
    
    // 获取设备管理器和动作管理器
    device_manager_t* dm = device_manager_get_instance();
    if (!dm) {
        printf("温度传感器初始化失败：无法获取设备管理器\n");
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 获取动作管理器并关联设备管理器
    action_manager_t* am = action_manager_get_instance();
    if (am) {
        printf("温度传感器初始化：关联动作管理器与设备管理器\n");
        action_manager_set_device_manager(am, dm);
    } else {
        printf("温度传感器初始化：警告 - 无法获取动作管理器\n");
    }
    
    // 分配内存
    dev_data->memory = device_memory_create(
        regions, 
        TEMP_REGION_COUNT, 
        am,  // 将动作管理器作为监视器传递给内存
        DEVICE_TYPE_TEMP_SENSOR, 
        instance->dev_id
    );
    
    if (!dev_data->memory) {
        printf("温度传感器初始化失败：无法分配内存\n");
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 设置初始温度值 (25°C = 0x19)
    uint32_t temp_value = 0x19;
    device_memory_write(dev_data->memory, TEMP_REG, temp_value);
    
    // 设置默认配置
    uint32_t config_value = 0x00;  // 默认配置：正常模式，报警禁用
    device_memory_write(dev_data->memory, CONFIG_REG, config_value);
    

    
    // 设置设备私有数据
    instance->priv_data = dev_data;
    
    // 获取规则管理器
    printf("温度传感器初始化：获取规则管理器\n");
    device_rule_manager_t* rule_manager = temp_sensor_get_rule_manager(instance);
    if (!rule_manager) {
        printf("温度传感器初始化失败：无法获取规则管理器\n");
        device_memory_destroy(dev_data->memory);
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    // 设置设备规则
    printf("温度传感器初始化：设置设备规则\n");
    int rule_count = setup_device_rules(rule_manager, DEVICE_TYPE_TEMP_SENSOR);
    printf("温度传感器初始化：已加载 %d 个规则\n", rule_count);
    dev_data->rule_count = rule_count;
    
    printf("温度传感器初始化完成\n");
    return 0;
}

// 读取温度传感器寄存器
int temp_sensor_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) {
        printf("DEBUG: temp_sensor_read - 无效参数: instance=%p, value=%p\n", 
               instance, value);
        return -1;
    }
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    if (!dev_data) {
        printf("DEBUG: temp_sensor_read - 无效设备数据: priv_data为NULL\n");
        return -1;
    }
    
    if (!dev_data->memory) {
        printf("DEBUG: temp_sensor_read - 无效设备内存: memory为NULL\n");
        return -1;
    }
    
    printf("DEBUG: temp_sensor_read - 温度传感器设备: ID=%d, memory=%p, region_count=%d\n",
           instance->dev_id, dev_data->memory, dev_data->memory->region_count);
    
    // 检查内存结构有效性
    if (dev_data->memory->region_count <= 0 || !dev_data->memory->regions) {
        printf("严重错误: temp_sensor_read - 内存区域无效: region_count=%d, regions=%p\n",
              dev_data->memory->region_count, (void*)dev_data->memory->regions);
        return -1;
    }
    
    // 检查每个内存区域
    for (int i = 0; i < dev_data->memory->region_count; i++) {
        memory_region_t* region = &dev_data->memory->regions[i];
        printf("DEBUG: temp_sensor_read - 区域[%d]: 基址=0x%08X, 单位大小=%zu, 长度=%zu, 数据指针=%p\n",
               i, region->base_addr, region->unit_size, region->length, region->data);
        
        // 检查数据指针有效性
        if (!region->data) {
            printf("严重错误: temp_sensor_read - 区域[%d]的数据指针为NULL\n", i);
            return -1;
        }
        
        // 检查地址是否在当前区域范围内
        if (addr >= region->base_addr && addr < region->base_addr + (region->unit_size * region->length)) {
            printf("DEBUG: temp_sensor_read - 地址0x%08X位于区域[%d]内\n", addr, i);
        }
    }
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 直接从内存读取数据
    printf("DEBUG: temp_sensor_read - 准备读取地址 0x%08X\n", addr);
    int ret = device_memory_read(dev_data->memory, addr, value);
    
    // 调试输出
    printf("DEBUG: temp_sensor_read - 读取地址=0x%08X, 值=0x%08X, 返回值=%d\n", 
           addr, ret == 0 ? *value : 0, ret);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入温度传感器寄存器
int temp_sensor_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) {
        printf("DEBUG: temp_sensor_write - 无效参数: instance=%p\n", instance);
        return -1;
    }
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    if (!dev_data || !dev_data->memory) {
        printf("DEBUG: temp_sensor_write - 无效设备数据: dev_data=%p, memory=%p\n", 
               dev_data, dev_data ? dev_data->memory : NULL);
        return -1;
    }
    
    printf("DEBUG: temp_sensor_write - 温度传感器设备: ID=%d, memory=%p, region_count=%d\n",
           instance->dev_id, dev_data->memory, dev_data->memory->region_count);
           
    // 检查内存结构有效性
    if (dev_data->memory->region_count <= 0 || !dev_data->memory->regions) {
        printf("严重错误: temp_sensor_write - 内存区域无效: region_count=%d, regions=%p\n",
              dev_data->memory->region_count, (void*)dev_data->memory->regions);
        return -1;
    }
    
    // 检查每个内存区域
    for (int i = 0; i < dev_data->memory->region_count; i++) {
        memory_region_t* region = &dev_data->memory->regions[i];
        printf("DEBUG: temp_sensor_write - 区域[%d]: 基址=0x%08X, 单位大小=%zu, 长度=%zu, 数据指针=%p\n",
               i, region->base_addr, region->unit_size, region->length, region->data);
        
        // 检查数据指针有效性
        if (!region->data) {
            printf("严重错误: temp_sensor_write - 区域[%d]的数据指针为NULL\n", i);
            return -1;
        }
        
        // 检查地址是否在当前区域范围内
        if (addr >= region->base_addr && addr < region->base_addr + (region->unit_size * region->length)) {
            printf("DEBUG: temp_sensor_write - 地址0x%08X位于区域[%d]内\n", addr, i);
        }
    }
    
    pthread_mutex_lock(&dev_data->mutex);
    
    // 写入设备内存
    printf("DEBUG: temp_sensor_write - 开始写入地址 0x%08X, 值=0x%08X, 内存指针=%p\n", 
           addr, value, dev_data->memory);
    int ret = device_memory_write(dev_data->memory, addr, value);
    printf("DEBUG: temp_sensor_write - 写入完成，返回值=%d\n", ret);
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 读取缓冲区
int temp_sensor_read_buffer(device_instance_t* instance, uint32_t addr, uint8_t* buffer, size_t length) {
    if (!instance || !buffer) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
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
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
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
    if (!instance || !instance->priv_data) {
        printf("温度传感器销毁失败：无效的设备实例\n");
        return;
    }
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    
    printf("销毁温度传感器设备 ID=%d\n", instance->dev_id);
    
    // 销毁内存
    if (dev_data->memory) {
        printf("销毁温度传感器内存\n");
        device_memory_destroy(dev_data->memory);
        dev_data->memory = NULL;
    }
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放私有数据
    free(dev_data);
    instance->priv_data = NULL;
    
    printf("温度传感器设备销毁完成\n");
}

// 配置内存区域
int temp_sensor_configure_memory(device_instance_t* instance, memory_region_config_t* configs, int config_count) {
    if (!instance || !configs || config_count <= 0) return -1;
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
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
        .get_memory = temp_sensor_get_memory,
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
    if (!instance || !instance->priv_data || !targets) {
        printf("DEBUG: temp_sensor_add_rule - 无效参数: instance=%p, priv_data=%p, targets=%p\n", 
               instance, instance ? instance->priv_data : NULL, targets);
        return -1;
    }
    
    temp_sensor_device_t* dev_data = (temp_sensor_device_t*)instance->priv_data;
    
    // 检查规则数量是否已达上限
    if (dev_data->rule_count >= 8) {
        printf("DEBUG: temp_sensor_add_rule - 温度传感器规则数量已达上限: %d\n", dev_data->rule_count);
        return -1;
    }
    
    // 添加新规则
    device_rule_t* rule = &dev_data->device_rules[dev_data->rule_count];
    rule->addr = addr;
    rule->expected_value = expected_value;
    rule->expected_mask = expected_mask;
    
    // 检查并输出目标信息
    printf("DEBUG: 添加规则 - 地址=0x%08X, 预期值=0x%08X, 掩码=0x%08X, 目标数量=%d\n", 
           addr, expected_value, expected_mask, targets->count);
    
    for (int i = 0; i < targets->count; i++) {
        const action_target_t* target = &targets->targets[i];
        printf("DEBUG:   目标 #%d: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X, 掩码=0x%08X\n", 
               i, target->type, target->device_type, target->device_id, 
               target->target_addr, target->target_value, target->target_mask);
    }
    
    // 复制目标数组
    memcpy(&rule->targets, targets, sizeof(action_target_array_t));
    
    // 保存规则ID并递增计数器
    int rule_id = dev_data->rule_count;
    dev_data->rule_count++;
    
    printf("DEBUG: 温度传感器规则添加成功，ID=%d，当前规则总数=%d\n", 
           rule_id, dev_data->rule_count);
    
    return rule_id;
}
