// src/action_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "action_manager.h"

// 规则提供者链表节点
typedef struct rule_provider_node {
    const rule_provider_t* provider;
    struct rule_provider_node* next;
} rule_provider_node_t;

// 全局规则提供者链表头
static rule_provider_node_t* g_rule_providers = NULL;

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
    if (am->rules) {
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

// 加载所有规则
int action_manager_load_all_rules(action_manager_t* am) {
    if (!am) return -1;
    
    rule_provider_node_t* curr = g_rule_providers;
    while (curr) {
        if (curr->provider && curr->provider->get_rules && curr->provider->get_rule_count) {
            const rule_table_entry_t* rules = curr->provider->get_rules();
            int count = curr->provider->get_rule_count();
            
            if (rules && count > 0) {
                printf("Loading rules from provider: %s\n", curr->provider->provider_name);
                if (action_manager_add_rules_from_table(am, rules, count) != 0) {
                    printf("Failed to load rules from provider: %s\n", curr->provider->provider_name);
                    return -1;
                }
            }
        }
        curr = curr->next;
    }
    
    return 0;
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
        rule->trigger_addr = entry->trigger_addr;
        rule->expect_value = entry->expect_value;
        rule->type = entry->type;
        rule->target_addr = entry->target_addr;
        rule->action_value = entry->action_value;
        rule->priority = entry->priority;
        rule->callback = entry->callback;
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
    memcpy(&am->rules[am->rule_count], rule, sizeof(action_rule_t));
    am->rule_count++;
    
    pthread_mutex_unlock(&am->mutex);
    return 0;
}

void action_manager_remove_rule(action_manager_t* am, int rule_id) {
    if (!am) return;
    
    pthread_mutex_lock(&am->mutex);
    for (int i = 0; i < am->rule_count; i++) {
        if (am->rules[i].rule_id == rule_id) {
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

void action_manager_execute_rule(action_manager_t* am, action_rule_t* rule) {
    if (!am || !rule) return;
    
    switch (rule->type) {
        case ACTION_TYPE_WRITE:
            // 写入操作由调用者执行
            break;
            
        case ACTION_TYPE_SIGNAL:
            // 信号发送
            break;
            
        case ACTION_TYPE_CALLBACK:
            if (rule->callback) {
                rule->callback(&rule->action_value);
            }
            break;
    }
}