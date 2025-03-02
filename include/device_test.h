/**
 * @file device_test.h
 * @brief 设备测试框架接口
 */

#ifndef DEVICE_TEST_H
#define DEVICE_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "device_types.h"  // 包含定义了device_type_id_t的头文件
#include "device_registry.h"  // 包含定义了device_manager_t的头文件
#include "action_manager.h"  // 包含定义了action_manager_t的头文件

// 不再需要以下前向声明，由相应的头文件提供
// typedef struct device_instance device_instance_t;
// typedef struct device_manager device_manager_t;
// typedef struct action_manager action_manager_t;
// typedef struct global_monitor global_monitor_t;
// typedef int device_type_id_t;

/**
 * @brief 测试步骤类型
 */
typedef struct {
    const char* name;        /**< 步骤名称 */
    uint32_t reg_addr;       /**< 寄存器地址 */
    uint32_t write_value;    /**< 写入值 */
    uint32_t expected_value; /**< 期望值 */
    bool is_write;           /**< 是否为写操作 */
    const char* format;      /**< 值格式化字符串 */
    float value_scale;       /**< 值的缩放比例 */
} test_step_t;

/**
 * @brief 测试结果枚举
 */
typedef enum {
    TEST_RESULT_PASS,        /**< 测试通过 */
    TEST_RESULT_FAIL,        /**< 测试失败 */
    TEST_RESULT_ERROR        /**< 测试错误 */
} test_result_t;

/**
 * @brief 测试用例设置函数类型
 */
typedef void (*test_setup_func_t)(device_manager_t*, void*);

/**
 * @brief 测试用例清理函数类型
 */
typedef void (*test_cleanup_func_t)(device_manager_t*, void*);

/**
 * @brief 测试用例类型
 */
typedef struct {
    const char* name;                  /**< 测试用例名称 */
    int device_type;                   /**< 设备类型 */
    int device_id;                     /**< 设备ID */
    const test_step_t* steps;          /**< 测试步骤数组 */
    int step_count;                    /**< 测试步骤数量 */
    test_setup_func_t setup;           /**< 设置函数 */
    test_cleanup_func_t cleanup;       /**< 清理函数 */
    void* user_data;                   /**< 用户数据 */
} test_case_t;

/**
 * @brief 测试套件类型
 */
typedef struct {
    const char* name;                  /**< 测试套件名称 */
    const test_case_t** test_cases;    /**< 测试用例数组 */
    int test_case_count;               /**< 测试用例数量 */
} test_suite_t;

/**
 * @brief 运行单个测试用例
 * 
 * @param dm 设备管理器
 * @param test_case 测试用例
 * @return test_result_t 测试结果
 */
test_result_t run_test_case(device_manager_t* dm, const test_case_t* test_case);

/**
 * @brief 运行测试套件
 * 
 * @param dm 设备管理器
 * @param suite 测试套件
 * @return int 成功的测试用例数
 */
int run_test_suite(device_manager_t* dm, const test_suite_t* suite);

/**
 * @brief 初始化测试环境
 * 
 * @param dm 设备管理器
 * @param gm 监视器（类型已改为void*）
 * @param am 动作管理器
 * @return int 成功返回0，失败返回负数
 */
int test_environment_init(device_manager_t** dm, void** gm, action_manager_t** am);

/**
 * @brief 清理测试环境
 * 
 * @param dm 设备管理器
 * @param gm 监视器（类型已改为void*）
 * @param am 动作管理器
 */
void test_environment_cleanup(device_manager_t* dm, void* gm, action_manager_t* am);

// 设备读写函数的声明
int device_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
int device_write(device_instance_t* instance, uint32_t addr, uint32_t value);

#endif /* DEVICE_TEST_H */ 