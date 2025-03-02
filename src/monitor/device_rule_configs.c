#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_rule_configs.h"
#include "device_types.h"
#include "action_manager.h"
#include "../plugins/flash/flash_device.h"
#include "../plugins/temp_sensor/temp_sensor.h"
#include "../plugins/fpga/fpga_device.h"

// 使用外部声明而不是重复定义
extern const device_rule_config_t flash_rule_configs[];
extern const int flash_rule_config_count;

extern const device_rule_config_t temp_sensor_rule_configs[];
extern const int temp_sensor_rule_config_count;

extern const device_rule_config_t fpga_rule_configs[];
extern const int fpga_rule_config_count;

// 从配置创建动作目标
action_target_t create_action_target_from_config(const device_rule_config_t* config) {
    action_target_t target;
    
    target.type = config->action_type;
    target.device_type = config->target_device_type;
    target.device_id = config->target_device_id;
    target.target_addr = config->target_addr;
    target.target_value = config->target_value;
    target.target_mask = config->target_mask;
    target.callback = config->callback;
    target.callback_data = config->callback_data;
    
    return target;
}

// 根据设备类型设置规则
int setup_device_rules(device_rule_manager_t* manager, device_type_id_t device_type) {
    if (!manager) return 0;
    
    const device_rule_config_t* configs = NULL;
    int config_count = 0;
    
    printf("DEBUG: setup_device_rules - 设备类型=%d\n", device_type);
    
    // 根据设备类型选择规则配置
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            configs = flash_rule_configs;
            config_count = flash_rule_config_count;
            printf("DEBUG: setup_device_rules - 选择Flash规则配置，数量=%d, 地址=%p\n", 
                   config_count, (void*)configs);
            break;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            configs = temp_sensor_rule_configs;
            config_count = temp_sensor_rule_config_count;
            printf("DEBUG: setup_device_rules - 选择温度传感器规则配置，数量=%d, 地址=%p\n", 
                   config_count, (void*)configs);
            break;
            
        case DEVICE_TYPE_FPGA:
            configs = fpga_rule_configs;
            config_count = fpga_rule_config_count;
            printf("DEBUG: setup_device_rules - 选择FPGA规则配置，数量=%d, 地址=%p\n", 
                   config_count, (void*)configs);
            break;
            
        default:
            printf("DEBUG: setup_device_rules - 未知设备类型=%d\n", device_type);
            return 0;
    }
    
    // 应用规则配置
    int rule_count = 0;
    for (int i = 0; i < config_count; i++) {
        printf("DEBUG: setup_device_rules - 处理规则配置 %d/%d\n", i+1, config_count);
        
        // 创建目标动作
        action_target_t target = create_action_target_from_config(&configs[i]);
        
        // 创建目标动作数组
        action_target_array_t* targets = (action_target_array_t*)malloc(sizeof(action_target_array_t));
        if (!targets) {
            printf("ERROR: setup_device_rules - 内存分配失败\n");
            continue;
        }
        memset(targets, 0, sizeof(action_target_array_t));
        action_target_add_to_array(targets, &target);
        
        // 添加规则
        if (device_rule_add(manager, configs[i].addr, configs[i].expected_value, 
                           configs[i].expected_mask, targets) == 0) {
            rule_count++;
            printf("DEBUG: setup_device_rules - 规则添加成功，当前规则数量=%d\n", rule_count);
        } else {
            printf("ERROR: setup_device_rules - 规则添加失败\n");
        }
        
        // 释放临时分配的内存
        free(targets);
    }
    
    printf("DEBUG: setup_device_rules - 设备类型=%d的规则设置完成，共添加%d条规则\n", 
           device_type, rule_count);
    
    return rule_count;
}

// 静态规则表
static rule_table_entry_t flash_rules[10];
static int flash_rule_table_count = 0;

static rule_table_entry_t temp_sensor_rules[10];
static int temp_sensor_rule_table_count = 0;

static rule_table_entry_t fpga_rules[10];
static int fpga_rule_table_count = 0;

// 初始化规则表
static void init_rule_tables(void) {
    static int initialized = 0;
    if (initialized) {
        printf("规则表已初始化，跳过\n");
        return;
    }
    
    printf("开始初始化规则表...\n");
    
    // 初始化Flash规则表
    printf("初始化Flash规则表...\n");
    printf("Flash规则配置数量: %d\n", flash_rule_config_count);
    for (int i = 0; i < flash_rule_config_count && i < 10; i++) {
        printf("处理Flash规则配置 %d/%d\n", i+1, flash_rule_config_count);
        action_target_array_t targets;
        memset(&targets, 0, sizeof(action_target_array_t));
        
        action_target_t target = create_action_target_from_config(&flash_rule_configs[i]);
        printf("创建目标动作: type=%d, device_type=%d, addr=0x%x\n", 
               target.type, target.device_type, target.target_addr);
        action_target_add_to_array(&targets, &target);
        
        rule_trigger_t trigger = rule_trigger_create(
            flash_rule_configs[i].addr,
            flash_rule_configs[i].expected_value,
            flash_rule_configs[i].expected_mask
        );
        printf("创建触发条件: addr=0x%x, value=0x%x, mask=0x%x\n", 
               trigger.trigger_addr, trigger.expected_value, trigger.expected_mask);
        
        char name[32];
        snprintf(name, sizeof(name), "Flash_Rule_%d", i);
        
        // 创建一个临时的rule_table_entry_t，然后复制到flash_rules[i]
        printf("创建规则表项: name=%s\n", name);
        rule_table_entry_t* temp_entry = rule_table_entry_create(strdup(name), trigger, &targets, 100);
        if (temp_entry) {
            printf("规则表项创建成功: %p\n", temp_entry);
            flash_rules[i] = *temp_entry;
            // 不要释放temp_entry->name，因为它已经被复制到flash_rules[i]
            free(temp_entry);
            flash_rule_table_count++;
        } else {
            printf("规则表项创建失败\n");
        }
    }
    printf("Flash规则表初始化完成，共 %d 条规则\n", flash_rule_table_count);
    printf("Flash规则表地址: %p\n", flash_rules);
    
    // 初始化温度传感器规则表
    printf("初始化温度传感器规则表...\n");
    printf("温度传感器规则配置数量: %d\n", temp_sensor_rule_config_count);
    for (int i = 0; i < temp_sensor_rule_config_count && i < 10; i++) {
        printf("处理温度传感器规则配置 %d/%d\n", i+1, temp_sensor_rule_config_count);
        action_target_array_t targets;
        memset(&targets, 0, sizeof(action_target_array_t));
        
        action_target_t target = create_action_target_from_config(&temp_sensor_rule_configs[i]);
        printf("创建目标动作: type=%d, device_type=%d, addr=0x%x\n", 
               target.type, target.device_type, target.target_addr);
        action_target_add_to_array(&targets, &target);
        
        rule_trigger_t trigger = rule_trigger_create(
            temp_sensor_rule_configs[i].addr,
            temp_sensor_rule_configs[i].expected_value,
            temp_sensor_rule_configs[i].expected_mask
        );
        printf("创建触发条件: addr=0x%x, value=0x%x, mask=0x%x\n", 
               trigger.trigger_addr, trigger.expected_value, trigger.expected_mask);
        
        char name[32];
        snprintf(name, sizeof(name), "TempSensor_Rule_%d", i);
        
        // 创建一个临时的rule_table_entry_t，然后复制到temp_sensor_rules[i]
        printf("创建规则表项: name=%s\n", name);
        rule_table_entry_t* temp_entry = rule_table_entry_create(strdup(name), trigger, &targets, 100);
        if (temp_entry) {
            printf("规则表项创建成功: %p\n", temp_entry);
            temp_sensor_rules[i] = *temp_entry;
            // 不要释放temp_entry->name，因为它已经被复制到temp_sensor_rules[i]
            free(temp_entry);
            temp_sensor_rule_table_count++;
        } else {
            printf("规则表项创建失败\n");
        }
    }
    printf("温度传感器规则表初始化完成，共 %d 条规则\n", temp_sensor_rule_table_count);
    printf("温度传感器规则表地址: %p\n", temp_sensor_rules);
    
    // 初始化FPGA规则表
    printf("初始化FPGA规则表...\n");
    printf("FPGA规则配置数量: %d\n", fpga_rule_config_count);
    for (int i = 0; i < fpga_rule_config_count && i < 10; i++) {
        printf("处理FPGA规则配置 %d/%d\n", i+1, fpga_rule_config_count);
        action_target_array_t targets;
        memset(&targets, 0, sizeof(action_target_array_t));
        
        action_target_t target = create_action_target_from_config(&fpga_rule_configs[i]);
        printf("创建目标动作: type=%d, device_type=%d, addr=0x%x\n", 
               target.type, target.device_type, target.target_addr);
        action_target_add_to_array(&targets, &target);
        
        rule_trigger_t trigger = rule_trigger_create(
            fpga_rule_configs[i].addr,
            fpga_rule_configs[i].expected_value,
            fpga_rule_configs[i].expected_mask
        );
        printf("创建触发条件: addr=0x%x, value=0x%x, mask=0x%x\n", 
               trigger.trigger_addr, trigger.expected_value, trigger.expected_mask);
        
        char name[32];
        snprintf(name, sizeof(name), "FPGA_Rule_%d", i);
        
        // 创建一个临时的rule_table_entry_t，然后复制到fpga_rules[i]
        printf("创建规则表项: name=%s\n", name);
        rule_table_entry_t* temp_entry = rule_table_entry_create(strdup(name), trigger, &targets, 100);
        if (temp_entry) {
            printf("规则表项创建成功: %p\n", temp_entry);
            fpga_rules[i] = *temp_entry;
            // 不要释放temp_entry->name，因为它已经被复制到fpga_rules[i]
            free(temp_entry);
            fpga_rule_table_count++;
        } else {
            printf("规则表项创建失败\n");
        }
    }
    printf("FPGA规则表初始化完成，共 %d 条规则\n", fpga_rule_table_count);
    printf("FPGA规则表地址: %p\n", fpga_rules);
    
    initialized = 1;
    printf("所有规则表初始化完成\n");
}

// 获取设备规则配置
const rule_table_entry_t* get_device_rules(device_type_id_t device_type, int* count) {
    // 确保规则表已初始化
    printf("获取设备类型 %d 的规则...\n", device_type);
    init_rule_tables();
    
    switch (device_type) {
        case DEVICE_TYPE_FLASH:
            if (count) *count = flash_rule_table_count;
            printf("返回 %d 条Flash规则，规则表地址: %p\n", flash_rule_table_count, flash_rules);
            // 打印每条规则的详细信息
            for (int i = 0; i < flash_rule_table_count; i++) {
                printf("Flash规则 %d: name=%s, trigger_addr=0x%x, targets.count=%d\n", 
                       i, flash_rules[i].name, flash_rules[i].trigger.trigger_addr, flash_rules[i].targets.count);
            }
            return flash_rules;
            
        case DEVICE_TYPE_TEMP_SENSOR:
            if (count) *count = temp_sensor_rule_table_count;
            printf("返回 %d 条温度传感器规则，规则表地址: %p\n", temp_sensor_rule_table_count, temp_sensor_rules);
            // 打印每条规则的详细信息
            for (int i = 0; i < temp_sensor_rule_table_count; i++) {
                printf("温度传感器规则 %d: name=%s, trigger_addr=0x%x, targets.count=%d\n", 
                       i, temp_sensor_rules[i].name, temp_sensor_rules[i].trigger.trigger_addr, temp_sensor_rules[i].targets.count);
            }
            return temp_sensor_rules;
            
        case DEVICE_TYPE_FPGA:
            if (count) *count = fpga_rule_table_count;
            printf("返回 %d 条FPGA规则，规则表地址: %p\n", fpga_rule_table_count, fpga_rules);
            // 打印每条规则的详细信息
            for (int i = 0; i < fpga_rule_table_count; i++) {
                printf("FPGA规则 %d: name=%s, trigger_addr=0x%x, targets.count=%d\n", 
                       i, fpga_rules[i].name, fpga_rules[i].trigger.trigger_addr, fpga_rules[i].targets.count);
            }
            return fpga_rules;
            
        default:
            if (count) *count = 0;
            printf("未知设备类型 %d，返回0条规则\n", device_type);
            return NULL;
    }
} 