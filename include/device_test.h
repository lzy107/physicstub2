#ifndef DEVICE_TEST_H
#define DEVICE_TEST_H

#include "device_registry.h"

// 测试步骤结构
typedef struct {
    const char* name;        // 步骤名称
    uint32_t reg_addr;       // 寄存器地址
    uint32_t write_value;    // 写入值（如果是写操作）
    uint32_t expect_value;   // 期望值（如果是读操作）
    int is_write;            // 是否为写操作
    const char* format;      // 输出格式
    float value_scale;       // 值的缩放因子
} test_step_t;

// 测试用例设置和清理函数类型
typedef void (*test_setup_fn)(device_instance_t*);
typedef void (*test_cleanup_fn)(device_instance_t*);

// 测试用例结构
typedef struct {
    const char* name;            // 测试用例名称
    device_type_id_t device_type;// 设备类型ID
    uint32_t device_id;          // 设备ID
    const test_step_t* steps;    // 测试步骤数组
    int step_count;              // 测试步骤数量
    test_setup_fn setup;         // 设置函数
    test_cleanup_fn cleanup;     // 清理函数
} test_case_t;

// 运行测试用例
int run_test_case(device_manager_t* dm, const test_case_t* test_case);

// 运行一组测试用例
int run_test_suite(device_manager_t* dm, const test_case_t* test_cases, int case_count);

#endif // DEVICE_TEST_H 