// src/action_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "action_manager.h"
#include "device_types.h"
#include "device_rule_configs.h"
#include "device_registry.h"
#include "device_memory.h"
#include "temp_sensor/temp_sensor.h"  // 添加温度传感器头文件

// 前向声明
static int action_target_count(action_target_array_t* targets);
static action_target_t* action_target_get(action_target_array_t* targets, int index);
static int execute_action_target(action_target_t* target, device_manager_t* dm);

// 简化的设备读取包装函数，适用于测试
static int device_read(device_instance_t* device, uint32_t addr, uint32_t* value) {
    if (!device || !value) {
        return -1;
    }
    
    // 对于温度传感器，调用特定的读函数
    if (device->dev_id == 0) {  // 这里简化为只有一个温度传感器设备
        return temp_sensor_read(device, addr, value);
    }
    
    return -1;
}

// 规则提供者链表节点
typedef struct rule_provider_node {
    const rule_provider_t* provider;
    struct rule_provider_node* next;
} rule_provider_node_t;

// 全局规则提供者链表头
static rule_provider_node_t* g_rule_providers = NULL;

// 创建目标动作数组
action_target_array_t* action_target_array_create(int initial_capacity) {
    if (initial_capacity <= 0 || initial_capacity > MAX_ACTION_TARGETS) {
        initial_capacity = MAX_ACTION_TARGETS; // 使用最大容量
    }
    
    action_target_array_t* array = (action_target_array_t*)calloc(1, sizeof(action_target_array_t));
    if (!array) return NULL;
    
    // 初始化数组
    memset(array, 0, sizeof(action_target_array_t));
    
    return array;
}

// 初始化目标动作数组
void action_target_array_init(action_target_array_t* array) {
    if (!array) return;
    memset(array, 0, sizeof(action_target_array_t));
}

// 添加目标动作到数组
void action_target_add_to_array(action_target_array_t* array, const action_target_t* target) {
    if (!array || !target) return;
    
    // 检查是否已达到最大容量
    if (array->count >= MAX_ACTION_TARGETS) {
        printf("Error: Action target array is full (max %d targets)\n", MAX_ACTION_TARGETS);
        return;
    }
    
    // 复制目标动作到数组
    memcpy(&array->targets[array->count], target, sizeof(action_target_t));
    array->count++;
}

// 销毁目标动作数组
void action_target_array_destroy(action_target_array_t* array) {
    if (!array) return;
    
    // 数组是直接包含在结构中的，不需要释放内存
    // 只需要清零即可
    memset(array, 0, sizeof(action_target_array_t));
}

// 创建单个目标动作
action_target_t* action_target_create(action_type_t type, device_type_id_t device_type, int device_id,
                                   uint32_t addr, uint32_t value, uint32_t mask, 
                                   action_callback_t callback, void* callback_data) {
    action_target_t* target = (action_target_t*)malloc(sizeof(action_target_t));
    if (!target) return NULL;
    
    target->type = type;
    target->device_type = device_type;
    target->device_id = device_id;
    target->target_addr = addr;
    target->target_value = value;
    target->target_mask = mask;
    target->callback = callback;
    target->callback_data = callback_data;
    
    return target;
}

// 创建规则触发条件
rule_trigger_t rule_trigger_create(uint32_t addr, uint32_t value, uint32_t mask) {
    rule_trigger_t trigger;
    trigger.trigger_addr = addr;
    trigger.expected_value = value;
    trigger.expected_mask = mask;
    return trigger;
}

// 创建规则表项
rule_table_entry_t* rule_table_entry_create(const char* name, rule_trigger_t trigger, 
                                          const action_target_array_t* targets, int priority) {
    rule_table_entry_t* entry = (rule_table_entry_t*)calloc(1, sizeof(rule_table_entry_t));
    if (!entry) return NULL;
    
    // 复制名称
    if (name) {
        char* name_copy = strdup(name);
        if (!name_copy) {
            free(entry);
            return NULL;
        }
        entry->name = name_copy;
    } else {
        entry->name = "Unnamed Rule";
    }
    
    // 设置触发条件和优先级
    entry->trigger = trigger;
    entry->priority = priority;
    
    // 复制目标数组
    if (targets) {
        memcpy(&entry->targets, targets, sizeof(action_target_array_t));
    } else {
        memset(&entry->targets, 0, sizeof(action_target_array_t));
    }
    
    return entry;
}

// 销毁规则表项
void rule_table_entry_destroy(rule_table_entry_t* entry) {
    if (!entry) return;
    
    // 释放名称
    if (entry->name && strcmp(entry->name, "Unnamed Rule") != 0) {
        free((void*)entry->name);
    }
    
    // 释放目标处理动作数组
    action_target_array_destroy(&entry->targets);
    
    // 释放表项本身
    free(entry);
}

// 复制目标处理动作数组
static action_target_array_t* action_target_copy_array(const action_target_t* src) {
    if (!src) return NULL;
    
    action_target_array_t* array = (action_target_array_t*)malloc(sizeof(action_target_array_t));
    if (!array) return NULL;
    
    // 初始化数组
    memset(array, 0, sizeof(action_target_array_t));
    
    // 计算源数组中的元素数量（假设最多MAX_ACTION_TARGETS个元素）
    int count = 0;
    for (count = 0; count < MAX_ACTION_TARGETS; count++) {
        // 如果遇到空元素，停止计数
        if (src[count].type == ACTION_TYPE_NONE) {
            break;
        }
    }
    
    // 复制元素
    memcpy(array->targets, src, count * sizeof(action_target_t));
    array->count = count;
    
    return array;
}

action_manager_t* action_manager_create(void) {
    action_manager_t* am = (action_manager_t*)calloc(1, sizeof(action_manager_t));
    if (!am) return NULL;
    
    pthread_mutex_init(&am->mutex, NULL);
    am->rules = NULL;  // 初始化为NULL，而不是分配0大小的内存
    am->rule_count = 0;
    
    return am;
}

void action_manager_destroy(action_manager_t* am) {
    if (!am) return;
    
    pthread_mutex_lock(&am->mutex);
    
    // 清理所有规则
    if (am->rules) {
        for (int i = 0; i < am->rule_count; i++) {
            if (am->rules[i].name && strcmp(am->rules[i].name, "Unnamed Rule") != 0) {
                free((void*)am->rules[i].name);
            }
            action_target_array_destroy(&am->rules[i].targets);
        }
        free(am->rules);
        am->rules = NULL;
    }
    
    pthread_mutex_unlock(&am->mutex);
    pthread_mutex_destroy(&am->mutex);
    free(am);
}

// 注册规则提供者
void action_manager_register_provider(const rule_provider_t* provider) {
    if (!provider) return;
    
    rule_provider_node_t* node = (rule_provider_node_t*)malloc(sizeof(rule_provider_node_t));
    if (!node) return;
    
    node->provider = provider;
    node->next = g_rule_providers;
    g_rule_providers = node;
}

// 修改加载规则的函数，使用全局规则配置
int action_manager_load_all_rules(action_manager_t* am) {
    if (!am) return -1;
    
    // 使用全局规则配置
    int total_rules = 0;
    
    printf("开始加载Flash设备规则...\n");
    printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
    // 加载 Flash 设备规则
    int count;
    const rule_table_entry_t* rules = get_device_rules(DEVICE_TYPE_FLASH, &count);
    if (rules && count > 0) {
        printf("找到 %d 条Flash设备规则\n", count);
        printf("规则表地址: %p\n", rules);
        int result = action_manager_add_rules_from_table(am, rules, count);
        if (result == 0) {
            total_rules += count;
            printf("成功加载 %d 条Flash设备规则\n", count);
            printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
        } else {
            printf("加载Flash设备规则失败\n");
        }
    } else {
        printf("未找到Flash设备规则\n");
    }
    
    printf("开始加载温度传感器设备规则...\n");
    printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
    // 加载温度传感器设备规则
    rules = get_device_rules(DEVICE_TYPE_TEMP_SENSOR, &count);
    if (rules && count > 0) {
        printf("找到 %d 条温度传感器设备规则\n", count);
        printf("规则表地址: %p\n", rules);
        int result = action_manager_add_rules_from_table(am, rules, count);
        if (result == 0) {
            total_rules += count;
            printf("成功加载 %d 条温度传感器设备规则\n", count);
            printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
        } else {
            printf("加载温度传感器设备规则失败\n");
        }
    } else {
        printf("未找到温度传感器设备规则\n");
    }
    
    printf("开始加载FPGA设备规则...\n");
    printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
    // 加载 FPGA 设备规则
    rules = get_device_rules(DEVICE_TYPE_FPGA, &count);
    if (rules && count > 0) {
        printf("找到 %d 条FPGA设备规则\n", count);
        printf("规则表地址: %p\n", rules);
        int result = action_manager_add_rules_from_table(am, rules, count);
        if (result == 0) {
            total_rules += count;
            printf("成功加载 %d 条FPGA设备规则\n", count);
            printf("动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
        } else {
            printf("加载FPGA设备规则失败\n");
        }
    } else {
        printf("未找到FPGA设备规则\n");
    }
    
    printf("总共加载了 %d 条规则\n", total_rules);
    return total_rules;
}

// 从规则表批量添加规则
int action_manager_add_rules_from_table(action_manager_t* am, const rule_table_entry_t* table, int count) {
    if (!am || !table || count <= 0) return -1;
    
    printf("开始从规则表添加规则，表地址: %p, 规则数量: %d\n", table, count);
    printf("当前动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
    
    pthread_mutex_lock(&am->mutex);
    
    // 分配新的规则数组
    printf("准备使用realloc分配内存，当前rules=%p, 新大小=%zu\n", 
           am->rules, (am->rule_count + count) * sizeof(action_rule_t));
    
    action_rule_t* new_rules;
    if (am->rules == NULL) {
        // 如果rules为NULL，使用malloc而不是realloc
        printf("rules为NULL，使用malloc分配内存\n");
        new_rules = (action_rule_t*)malloc((am->rule_count + count) * sizeof(action_rule_t));
    } else {
        // 否则使用realloc
        new_rules = (action_rule_t*)realloc(am->rules, 
            (am->rule_count + count) * sizeof(action_rule_t));
    }
        
    if (!new_rules) {
        printf("内存分配失败，无法分配内存\n");
        pthread_mutex_unlock(&am->mutex);
        return -1;
    }
    
    printf("内存分配成功，新rules地址=%p\n", new_rules);
    am->rules = new_rules;
    
    // 添加规则表中的所有规则
    for (int i = 0; i < count; i++) {
        printf("处理规则 %d/%d\n", i+1, count);
        action_rule_t* rule = &am->rules[am->rule_count + i];
        const rule_table_entry_t* entry = &table[i];
        
        // 生成规则ID
        static int next_rule_id = 1;
        rule->rule_id = next_rule_id++;
        
        // 复制规则内容
        printf("复制规则内容: name=%s, trigger_addr=0x%x\n", 
               entry->name ? entry->name : "NULL", entry->trigger.trigger_addr);
        
        // 复制名称字符串而不是仅复制指针
        if (entry->name && strcmp(entry->name, "Unnamed Rule") != 0) {
            rule->name = strdup(entry->name);
            if (!rule->name) {
                // 如果分配失败，使用默认名称
                rule->name = "Unnamed Rule";
            }
        } else {
            rule->name = "Unnamed Rule";
        }
        
        rule->trigger = entry->trigger;
        rule->priority = entry->priority;
        
        // 直接复制目标处理动作数组
        printf("复制目标处理动作数组: count=%d\n", entry->targets.count);
        memcpy(&rule->targets, &entry->targets, sizeof(action_target_array_t));
    }
    
    am->rule_count += count;
    printf("规则添加完成，当前动作管理器状态: rules=%p, rule_count=%d\n", am->rules, am->rule_count);
    
    pthread_mutex_unlock(&am->mutex);
    return 0;
}

int action_manager_add_rule(action_manager_t* am, action_rule_t* rule) {
    if (!am || !rule) return -1;
    
    pthread_mutex_lock(&am->mutex);
    
    action_rule_t* new_rules;
    if (am->rules == NULL) {
        // 如果rules为NULL，使用malloc而不是realloc
        new_rules = (action_rule_t*)malloc(sizeof(action_rule_t));
    } else {
        // 否则使用realloc
        new_rules = (action_rule_t*)realloc(am->rules, 
            (am->rule_count + 1) * sizeof(action_rule_t));
    }
        
    if (!new_rules) {
        pthread_mutex_unlock(&am->mutex);
        return -1;
    }
    
    am->rules = new_rules;
    
    // 复制规则内容
    action_rule_t* new_rule = &am->rules[am->rule_count];
    new_rule->rule_id = rule->rule_id;
    
    // 复制名称字符串而不是仅复制指针
    if (rule->name && strcmp(rule->name, "Unnamed Rule") != 0) {
        new_rule->name = strdup(rule->name);
        if (!new_rule->name) {
            // 如果分配失败，使用默认名称
            new_rule->name = "Unnamed Rule";
        }
    } else {
        new_rule->name = "Unnamed Rule";
    }
    
    new_rule->trigger = rule->trigger;
    new_rule->priority = rule->priority;
    
    // 直接复制目标处理动作数组
    memcpy(&new_rule->targets, &rule->targets, sizeof(action_target_array_t));
    
    am->rule_count++;
    
    pthread_mutex_unlock(&am->mutex);
    return 0;
}

void action_manager_remove_rule(action_manager_t* am, int rule_id) {
    if (!am) return;
    
    pthread_mutex_lock(&am->mutex);
    for (int i = 0; i < am->rule_count; i++) {
        if (am->rules[i].rule_id == rule_id) {
            // 清理目标处理动作数组
            action_target_array_destroy(&am->rules[i].targets);
            
            // 移动后面的规则
            if (i < am->rule_count - 1) {
                memmove(&am->rules[i], &am->rules[i + 1], 
                    (am->rule_count - i - 1) * sizeof(action_rule_t));
            }
            am->rule_count--;
            break;
        }
    }
    pthread_mutex_unlock(&am->mutex);
}

/**
 * 执行目标处理动作
 * 
 * @param target 目标处理动作
 * @param dm 设备管理器
 * @return 成功返回0，失败返回非0
 */
static int execute_action_target(action_target_t* target, device_manager_t* dm) {
    struct timeval tv;
    time_t now;
    
    if (!target || !dm) {
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    
    // 记录执行开始
    printf("[%ld.%06ld] execute_action_target - 开始执行目标, 目标=%p, 设备管理器=%p\n",
               tv.tv_sec, tv.tv_usec, target, dm);
    
    // 输出目标详细信息
    printf("[%ld.%06ld] execute_action_target - 目标详情: 类型=%d, 地址=0x%08X, 值=0x%08X, 掩码=0x%08X\n",
          tv.tv_sec, tv.tv_usec, target->type, target->target_addr, target->target_value, target->target_mask);
    
    // 获取目标对应的设备
    device_instance_t* device = device_manager_get_device_by_addr(dm, target->target_addr);
    gettimeofday(&tv, NULL);
    
    if (!device) {
        printf("[%ld.%06ld] execute_action_target - 错误: 找不到匹配地址0x%08X的设备\n",
               tv.tv_sec, tv.tv_usec, target->target_addr);
        
        // 尝试列出所有已注册设备以进行调试
        printf("[%ld.%06ld] execute_action_target - 尝试列出所有已注册设备:\n", tv.tv_sec, tv.tv_usec);
        device_manager_dump_devices(dm);
        
        return -1;
    }
    
    // 写入目标值
    uint32_t offset = target->target_addr;  // 简化，直接使用地址
    
    printf("[%ld.%06ld] execute_action_target - 计算偏移: 偏移=0x%08X, 目标地址=0x%08X\n",
           tv.tv_sec, tv.tv_usec, offset, target->target_addr);
    
    // 在日志中记录将要执行的写入操作
    printf("[%ld.%06ld] execute_action_target - 将向地址0x%08X (偏移0x%08X)写入值0x%08X\n",
          tv.tv_sec, tv.tv_usec, target->target_addr, offset, target->target_value);
    
    // 记录当前时间以进行调试
    now = time(NULL);
    printf("[%ld.%06ld] execute_action_target - 当前时间: %s",
           tv.tv_sec, tv.tv_usec, ctime(&now));
    
    // 简化处理 - 直接写入设备的目标地址
    // 对于温度传感器的情况，使用特定的处理函数
    if (target->device_type == DEVICE_TYPE_TEMP_SENSOR) {
        // 对于温度传感器，调用特定的写函数
        int write_result = temp_sensor_write(device, target->target_addr, target->target_value);
        
        gettimeofday(&tv, NULL);
        printf("[%ld.%06ld] execute_action_target - 温度传感器写入结果: %d\n",
               tv.tv_sec, tv.tv_usec, write_result);
        
        // 验证写入是否成功
        uint32_t current_value = 0;
        if (temp_sensor_read(device, target->target_addr, &current_value) == 0) {
            printf("[%ld.%06ld] execute_action_target - 当前值: 0x%08X\n",
                   tv.tv_sec, tv.tv_usec, current_value);
        } else {
            printf("[%ld.%06ld] execute_action_target - 无法读取当前内存值\n", tv.tv_sec, tv.tv_usec);
        }
        
        return write_result;
    }
    
    // 记录当前时间以进行调试
    now = time(NULL);
    printf("[%ld.%06ld] execute_action_target - 写入完成时间: %s",
           tv.tv_sec, tv.tv_usec, ctime(&now));
    
    // 读取写入后的值进行验证
    uint32_t read_value = 0;
    if (device_read(device, offset, &read_value) == 0) {
        printf("[%ld.%06ld] execute_action_target - 验证写入: 目标值=0x%08X, 读取值=0x%08X %s\n",
               tv.tv_sec, tv.tv_usec, target->target_value, read_value, 
               (read_value == target->target_value) ? "成功" : "失败");
    } else {
        printf("[%ld.%06ld] execute_action_target - 无法验证写入，读取失败\n", tv.tv_sec, tv.tv_usec);
    }
    
    printf("[%ld.%06ld] execute_action_target - 写入内存成功\n", tv.tv_sec, tv.tv_usec);
    return 0;
}

/**
 * 执行指定的规则
 * 
 * @param am 动作管理器
 * @param rule 规则
 * @param dm 设备管理器
 */
void action_manager_execute_rule(action_manager_t* am, action_rule_t* rule, device_manager_t* dm) {
    struct timeval tv, start_time, end_time;
    int target_count;
    
    if (!am || !rule || !dm) {
        printf("错误: action_manager_execute_rule - 参数无效 (am=%p, rule=%p, dm=%p)\n", am, rule, dm);
        return;
    }
    
    gettimeofday(&tv, NULL);
    
    printf("[%ld.%06ld] action_manager_execute_rule - 开始执行规则, ID=%d, 名称=%s\n",
          tv.tv_sec, tv.tv_usec, rule->rule_id, rule->name ? rule->name : "未命名");
    
    // 获取规则中目标处理动作数量
    target_count = action_target_count(&rule->targets);
    
    printf("[%ld.%06ld] action_manager_execute_rule - 目标处理动作数量: %d\n",
           tv.tv_sec, tv.tv_usec, target_count);
    
    // 输出触发器信息
    printf("[%ld.%06ld] action_manager_execute_rule - 规则触发器: 地址=0x%08X, 值=0x%08X, 掩码=0x%08X\n",
           tv.tv_sec, tv.tv_usec, rule->trigger.trigger_addr, rule->trigger.expected_value, rule->trigger.expected_mask);
    
    // 批量执行全部目标处理动作
    action_target_t* target;
    int i;
    
    for (i = 0; i < target_count; i++) {
        gettimeofday(&start_time, NULL);
        
        printf("[%ld.%06ld] action_manager_execute_rule - 执行目标处理动作 %d/%d\n",
               tv.tv_sec, tv.tv_usec, i+1, target_count);
        
        // 获取当前目标处理动作
        target = action_target_get(&rule->targets, i);
        if (!target) {
            continue;
        }
        
        // 输出目标详细信息
        printf("[%ld.%06ld] action_manager_execute_rule - 目标 %d 详情: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X, 掩码=0x%08X\n",
               tv.tv_sec, tv.tv_usec, i+1, target->type, target->device_type, target->device_id, 
               target->target_addr, target->target_value, target->target_mask);
        
        // 执行目标处理动作
        int result = execute_action_target(target, dm);
        if (result != 0) {
            printf("警告: 执行目标处理动作失败，返回值=%d\n", result);
        }
        
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
                        
        printf("[%ld.%06ld] action_manager_execute_rule - 目标处理动作 %d/%d 执行完成，耗时 %.6f 秒\n",
               end_time.tv_sec, end_time.tv_usec, i+1, target_count, elapsed);
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] action_manager_execute_rule - 规则执行完成\n", tv.tv_sec, tv.tv_usec);
}

/**
 * 获取目标数组中目标的数量
 * 
 * @param targets 目标数组
 * @return 目标数量
 */
static int action_target_count(action_target_array_t* targets) {
    if (!targets) {
        return 0;
    }
    return targets->count;
}

/**
 * 从目标数组中获取指定索引的目标
 * 
 * @param targets 目标数组
 * @param index 索引
 * @return 目标指针，如果索引无效则返回NULL
 */
static action_target_t* action_target_get(action_target_array_t* targets, int index) {
    if (!targets || index < 0 || index >= targets->count) {
        return NULL;
    }
    return &targets->targets[index];
}