#ifndef DEVICE_TEST_H
#define DEVICE_TEST_H

#include "device_types.h"

// 测试步骤结构
typedef struct {
    const char* name;           // 测试步骤名称
    uint32_t reg_addr;         // 要访问的寄存器地址
    uint32_t write_value;      // 要写入的值（如果是写操作）
    uint32_t expect_value;     // 期望读取的值（如果是读操作）
    int is_write;              // 1表示写操作，0表示读操作
    float value_scale;         // 值的缩放因子（用于温度等需要转换的值）
    const char* format;        // 打印格式（如"0x%02X"或"%.2f°C"）
} test_step_t;

// 设备测试用例结构
typedef struct {
    const char* name;                  // 测试用例名称
    device_type_id_t device_type;      // 设备类型
    int device_id;                     // 设备ID
    const test_step_t* steps;          // 测试步骤数组
    int step_count;                    // 测试步骤数量
    void (*setup)(device_instance_t*); // 测试前的设置函数（可选）
    void (*cleanup)(device_instance_t*); // 测试后的清理函数（可选）
} test_case_t;

// 运行单个测试用例
int run_test_case(device_manager_t* dm, const test_case_t* test_case);

// 运行一组测试用例
int run_test_suite(device_manager_t* dm, const test_case_t* test_cases, int case_count);

#endif // DEVICE_TEST_H 