// src/action_manager.c
#include <stdlib.h>
#include <string.h>
#include "action_manager.h"

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

action_rule_t* action_rule_create(uint32_t trigger_addr, uint32_t expect_value,
                                action_type_t type, uint32_t target_addr, 
                                uint32_t action_value) {
    action_rule_t* rule = (action_rule_t*)calloc(1, sizeof(action_rule_t));
    if (!rule) return NULL;
    
    static int next_rule_id = 1;
    rule->rule_id = next_rule_id++;
    rule->trigger_addr = trigger_addr;
    rule->expect_value = expect_value;
    rule->type = type;
    rule->target_addr = target_addr;
    rule->action_value = action_value;
    rule->priority = 0;  // 默认优先级
    
    return rule;
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
    // 深拷贝规则
    am->rules[am->rule_count].rule_id = rule->rule_id;
    am->rules[am->rule_count].trigger_addr = rule->trigger_addr;
    am->rules[am->rule_count].expect_value = rule->expect_value;
    am->rules[am->rule_count].type = rule->type;
    am->rules[am->rule_count].target_addr = rule->target_addr;
    am->rules[am->rule_count].action_value = rule->action_value;
    am->rules[am->rule_count].priority = rule->priority;
    am->rules[am->rule_count].callback = rule->callback;  // 函数指针可以直接复制
    
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
                rule->callback(&rule->action_value);  // 使用 action_value 作为回调数据
            }
            break;
    }
}