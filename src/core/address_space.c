// src/address_space.c
#include <stdlib.h>
#include "address_space.h"

address_space_t* address_space_create(void) {
    address_space_t* as = (address_space_t*)calloc(1, sizeof(address_space_t));
    if (!as) return NULL;
    
    pthread_rwlock_init(&as->lock, NULL);
    as->map = NULL;  // uthash初始化为NULL
    return as;
}

void address_space_destroy(address_space_t* as) {
    if (!as) return;
    
    // 获取写锁，确保没有其他线程在使用地址空间
    pthread_rwlock_wrlock(&as->lock);
    
    // 清理所有映射
    addr_map_t* current, *tmp;
    HASH_ITER(hh, as->map, current, tmp) {
        HASH_DEL(as->map, current);
        free(current);
    }
    as->map = NULL;  // 防止悬空指针
    
    // 释放写锁并销毁它
    pthread_rwlock_unlock(&as->lock);
    pthread_rwlock_destroy(&as->lock);
    
    // 最后释放地址空间结构
    free(as);
}

int address_space_read(address_space_t* as, uint32_t addr, uint32_t* value) {
    if (!as || !value) return -1;
    
    pthread_rwlock_rdlock(&as->lock);
    addr_map_t* item = NULL;
    HASH_FIND(hh, as->map, &addr, sizeof(uint32_t), item);
    if (item) {
        *value = item->value;
    } else {
        *value = 0;  // 未映射地址默认返回0
    }
    pthread_rwlock_unlock(&as->lock);
    
    return item ? 0 : -1;
}

int address_space_write(address_space_t* as, uint32_t addr, uint32_t value) {
    if (!as) return -1;
    
    pthread_rwlock_wrlock(&as->lock);
    addr_map_t* item = NULL;
    HASH_FIND(hh, as->map, &addr, sizeof(uint32_t), item);
    
    if (!item) {
        item = (addr_map_t*)calloc(1, sizeof(addr_map_t));
        if (!item) {
            pthread_rwlock_unlock(&as->lock);
            return -1;
        }
        item->addr = addr;
        HASH_ADD(hh, as->map, addr, sizeof(uint32_t), item);
    }
    
    item->value = value;
    pthread_rwlock_unlock(&as->lock);
    return 0;
}