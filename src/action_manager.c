// src/action_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "action_manager.h"
#include "device_types.h"

// 规则提供者链表节点
typedef struct rule_provider_node {
    const rule_provider_t* provider;
    struct rule_provider_node* next;
} rule_provider_node_t;

// 全局规则提供者链表头
static rule_provider_node_t* g_rule_providers = NULL;

// 创建目标动作数组
action_target_array_t* action_target_array_create(int initial_capacity) {
    if (initial_capacity <= 0) initial_capacity = 4; // 默认初始容量
    
    action_target_array_t* array = (action_target_array_t*)calloc(1, sizeof(action_target_array_t));
    if (!array) return NULL;
    
    array->targets = (action_target_t*)calloc(initial_capacity, sizeof(action_target_t));
    if (!array->targets) {
        free(array);
        return NULL;
    }
    
    array->count = 0;
    array->capacity = initial_capacity;
    
    return array;
}

// 初始化目标动作数组
void action_target_array_init(action_target_array_t* array) {
    if (!array) return;
    memset(array, 0, sizeof(action_target_array_t));
}

// 添加目标动作到数组
int action_target_array_add(action_target_array_t* array, const action_target_t* target) {
    if (!array || !target) return -1;
    
    // 检查是否已达到最大容量
    if (array->count >= MAX_ACTION_TARGETS) {
        printf("Error: Action target array is full (max %d targets)\n", MAX_ACTION_TARGETS);
        return -1;
    }
    
    // 复制目标动作到数组
    memcpy(&array->targets[array->count], target, sizeof(action_target_t));
    array->count++;
    
    return 0;
}

// 销毁目标动作数组
void action_target_array_destroy(action_target_array_t* array) {
    if (!array) return;
    
    if (array->targets) {
        free(array->targets);
    }
    
    free(array);
}

// 创建单个目标动作
action_target_t action_target_create(action_type_t type, device_type_id_t device_type, int device_id,
                                   uint32_t addr, uint32_t value, uint32_t mask, 
                                   action_callback_t callback, void* callback_data) {
    action_target_t target;
    
    target.type = type;
    target.device_type = device_type;
    target.device_id = device_id;
    target.target_addr = addr;
    target.target_value = value;
    target.target_mask = mask;
    target.callback = callback;
    target.callback_data = callback_data;
    
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
                                          action_target_array_t targets, int priority) {
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
    
    // 设置触发条件
    entry->trigger = trigger;
    
    // 设置目标处理动作数组
    entry->targets = targets;
    
    // 设置优先级
    entry->priority = priority;
    
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
static action_target_t* action_target_copy_array(const action_target_t* src) {
    if (!src) return NULL;
    
    action_target_t* head = (action_target_t*)malloc(MAX_ACTION_TARGETS * sizeof(action_target_t));
    if (!head) return NULL;
    
    memcpy(head, src, MAX_ACTION_TARGETS * sizeof(action_target_t));
    
    return head;
}

action_manager_t* action_manager_create(void) {
    action_manager_t* am = (action_manager_t*)calloc(1, sizeof(action_manager_t));
    if (!am) return NULL;
    
    pthread_mutex_init(&am->mutex, NULL);
    am->rules = NULL;
    am->rule_count = 0;
    
    return am;
}

void action_manager_destroy(action_manager_t* am) {
    if (!am) return;
    
    pthread_mutex_lock(&am->mutex);
    
    // 清理所有规则
    if (am->rules) {
        for (int i = 0; i < am->rule_count; i++) {
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
    
    // 加载 Flash 设备规则
    int count;
    const rule_table_entry_t* rules = get_device_rules(DEVICE_TYPE_FLASH, &count);
    if (rules && count > 0) {
        action_manager_add_rules_from_table(am, rules, count);
        total_rules += count;
    }
    
    // 加载温度传感器设备规则
    rules = get_device_rules(DEVICE_TYPE_TEMP_SENSOR, &count);
    if (rules && count > 0) {
        action_manager_add_rules_from_table(am, rules, count);
        total_rules += count;
    }
    
    // 加载 FPGA 设备规则
    rules = get_device_rules(DEVICE_TYPE_FPGA, &count);
    if (rules && count > 0) {
        action_manager_add_rules_from_table(am, rules, count);
        total_rules += count;
    }
    
    return total_rules;
}

// 从规则表批量添加规则
int action_manager_add_rules_from_table(action_manager_t* am, const rule_table_entry_t* table, int count) {
    if (!am || !table || count <= 0) return -1;
    
    pthread_mutex_lock(&am->mutex);
    
    // 分配新的规则数组
    action_rule_t* new_rules = (action_rule_t*)realloc(am->rules, 
        (am->rule_count + count) * sizeof(action_rule_t));
        
    if (!new_rules) {
        pthread_mutex_unlock(&am->mutex);
        return -1;
    }
    
    am->rules = new_rules;
    
    // 添加规则表中的所有规则
    for (int i = 0; i < count; i++) {
        action_rule_t* rule = &am->rules[am->rule_count + i];
        const rule_table_entry_t* entry = &table[i];
        
        // 生成规则ID
        static int next_rule_id = 1;
        rule->rule_id = next_rule_id++;
        
        // 复制规则内容
        rule->name = entry->name;
        rule->trigger = entry->trigger;
        rule->priority = entry->priority;
        
        // 复制目标处理动作数组
        rule->targets = action_target_copy_array(&entry->targets.targets[0]);
        if (!rule->targets) {
            // 如果复制失败，清理已分配的资源
            for (int j = 0; j < i; j++) {
                action_target_array_destroy(&am->rules[am->rule_count + j].targets);
            }
            pthread_mutex_unlock(&am->mutex);
            return -1;
        }
    }
    
    am->rule_count += count;
    
    pthread_mutex_unlock(&am->mutex);
    return 0;
}

int action_manager_add_rule(action_manager_t* am, action_rule_t* rule) {
    if (!am || !rule) return -1;
    
    pthread_mutex_lock(&am->mutex);
    
    action_rule_t* new_rules = (action_rule_t*)realloc(am->rules, 
        (am->rule_count + 1) * sizeof(action_rule_t));
        
    if (!new_rules) {
        pthread_mutex_unlock(&am->mutex);
        return -1;
    }
    
    am->rules = new_rules;
    
    // 复制规则内容
    action_rule_t* new_rule = &am->rules[am->rule_count];
    new_rule->rule_id = rule->rule_id;
    new_rule->name = rule->name;
    new_rule->trigger = rule->trigger;
    new_rule->priority = rule->priority;
    
    // 复制目标处理动作数组
    new_rule->targets = action_target_copy_array(&rule->targets.targets[0]);
    if (!new_rule->targets) {
        pthread_mutex_unlock(&am->mutex);
        return -1;
    }
    
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

// 执行单个目标处理动作
static void execute_action_target(action_target_t* target, device_manager_t* dm) {
    if (!target || !dm) return;
    
    switch (target->type) {
        case ACTION_TYPE_NONE:
            // 不执行任何操作
            break;
            
        case ACTION_TYPE_WRITE: {
            // 获取设备实例
            device_instance_t* instance = device_get(dm, target->device_type, target->device_id);
            if (!instance) return;
            
            // 获取设备操作接口
            device_ops_t* ops = &dm->types[target->device_type].ops;
            if (!ops || !ops->write) return;
            
            // 获取设备特定的互斥锁
            pthread_mutex_t* mutex = NULL;
            if (ops->get_mutex) {
                mutex = ops->get_mutex(instance);
            }
            
            // 如果无法获取设备特定的互斥锁，回退到使用设备类型锁
            if (!mutex) {
                device_type_t* type = &dm->types[target->device_type];
                mutex = &type->mutex;
            }
            
            // 获取互斥锁
            pthread_mutex_lock(mutex);
            
            // 如果有掩码，先读取当前值，然后应用掩码
            if (target->target_mask != 0xFFFFFFFF) {
                uint32_t current_value;
                if (ops->read && ops->read(instance, target->target_addr, &current_value) == 0) {
                    // 清除掩码位，然后设置新值
                    uint32_t new_value = (current_value & ~target->target_mask) | 
                                        (target->target_value & target->target_mask);
                    ops->write(instance, target->target_addr, new_value);
                }
            } else {
                // 直接写入新值
                ops->write(instance, target->target_addr, target->target_value);
            }
            
            // 释放互斥锁
            pthread_mutex_unlock(mutex);
            break;
        }
        
        case ACTION_TYPE_SIGNAL:
            // 信号发送逻辑
            break;
            
        case ACTION_TYPE_CALLBACK:
            if (target->callback) {
                target->callback(target->callback_data ? target->callback_data : &target->target_value);
            }
            break;
    }
}

// 执行规则
void action_manager_execute_rule(action_manager_t* am, action_rule_t* rule, device_manager_t* dm) {
    if (!am || !rule || !dm) return;
    
    // 执行所有目标处理动作
    for (int i = 0; i < rule->targets.count; i++) {
        action_target_t* target = &rule->targets.targets[i];
        execute_action_target(target, dm);
    }
}