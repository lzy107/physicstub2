// include/address_space.h
#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include <stdint.h>
#include <pthread.h>
#include "uthash.h"

// 地址-值映射结构
typedef struct {
    uint32_t addr;           // 地址
    uint32_t value;         // 值
    UT_hash_handle hh;      // uthash句柄
} addr_map_t;

// 地址空间结构
typedef struct {
    addr_map_t* map;        // 地址映射哈希表
    pthread_rwlock_t lock;   // 读写锁
} address_space_t;

// API函数声明
address_space_t* address_space_create(void);
void address_space_destroy(address_space_t* as);
int address_space_read(address_space_t* as, uint32_t addr, uint32_t* value);
int address_space_write(address_space_t* as, uint32_t addr, uint32_t value);

#endif