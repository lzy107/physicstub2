// src/global_monitor.c
#include <stdlib.h>
#include "global_monitor.h"

global_monitor_t* global_monitor_create(action_manager_t* am) {
    if (!am) return NULL;
    
    global_monitor_t* gm = (global_monitor_t*)calloc(1, sizeof(global_monitor_t));
    if (!gm) return NULL;
    
    pthread_mutex_init(&gm->mutex, NULL);
    gm->watches = NULL;
    gm->am = am;
    
    return gm;
}

void global_monitor_destroy(global_monitor_t* gm) {
    if (!gm) return;
    
    pthread_mutex_lock(&gm->mutex);
    watch_point_t* current, *tmp;
    HASH_ITER(hh, gm->watches, current, tmp) {
        HASH_DEL(gm->watches, current);
        free(current);
    }
    pthread_mutex_unlock(&gm->mutex);
    
    pthread_mutex_destroy(&gm->mutex);
    free(gm);
}

int global_monitor_add_watch(global_monitor_t* gm, uint32_t addr, 
                           uint32_t value, action_rule_t* rule) {
    if (!gm || !rule || !gm->am) return -1;
    
    pthread_mutex_lock(&gm->mutex);
    
    // 首先在 action_manager 中查找规则
    action_rule_t* stored_rule = NULL;
    for (int i = 0; i < gm->am->rule_count; i++) {
        if (gm->am->rules[i].rule_id == rule->rule_id) {
            stored_rule = &gm->am->rules[i];
            break;
        }
    }
    
    if (!stored_rule) {
        pthread_mutex_unlock(&gm->mutex);
        return -1;
    }
    
    watch_point_t* wp = NULL;
    HASH_FIND(hh, gm->watches, &addr, sizeof(uint32_t), wp);
    
    if (!wp) {
        wp = (watch_point_t*)calloc(1, sizeof(watch_point_t));
        if (!wp) {
            pthread_mutex_unlock(&gm->mutex);
            return -1;
        }
        wp->addr = addr;
        HASH_ADD(hh, gm->watches, addr, sizeof(uint32_t), wp);
    }
    
    wp->expect_value = value;
    wp->rule = stored_rule;  // 使用存储在 action_manager 中的规则
    
    pthread_mutex_unlock(&gm->mutex);
    return 0;
}

void global_monitor_remove_watch(global_monitor_t* gm, uint32_t addr) {
    if (!gm) return;
    
    pthread_mutex_lock(&gm->mutex);
    watch_point_t* wp;
    HASH_FIND(hh, gm->watches, &addr, sizeof(uint32_t), wp);
    if (wp) {
        HASH_DEL(gm->watches, wp);
        free(wp);
    }
    pthread_mutex_unlock(&gm->mutex);
}

void global_monitor_on_address_change(global_monitor_t* gm, uint32_t addr, uint32_t value) {
    if (!gm || !gm->am) return;
    
    pthread_mutex_lock(&gm->mutex);
    watch_point_t* wp;
    HASH_FIND(hh, gm->watches, &addr, sizeof(uint32_t), wp);
    
    if (wp && wp->expect_value == value && wp->rule) {
        action_manager_execute_rule(gm->am, wp->rule);
    }
    
    pthread_mutex_unlock(&gm->mutex);
}