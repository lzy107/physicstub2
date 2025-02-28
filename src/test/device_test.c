/**
 * @file device_test.c
 * @brief 设备测试框架实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>  // 添加数学库头文件，用于fabsf函数
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

/**
 * @brief 打印测试步骤结果
 * 
 * @param step 测试步骤
 * @param actual_value 实际值
 * @param pass 是否通过
 */
static void print_test_step_result(const test_step_t* step, uint32_t actual_value, bool pass) {
    char expected_str[64] = {0};
    char actual_str[64] = {0};
    
    if (step->format) {
        snprintf(expected_str, sizeof(expected_str), step->format, 
                 step->is_write ? step->write_value : step->expected_value);
        snprintf(actual_str, sizeof(actual_str), step->format, actual_value);
    } else {
        snprintf(expected_str, sizeof(expected_str), "0x%08X", 
                 step->is_write ? step->write_value : step->expected_value);
        snprintf(actual_str, sizeof(actual_str), "0x%08X", actual_value);
    }
    
    printf("  %-30s: %s [%s]\n", step->name, 
           pass ? "通过" : "失败", 
           step->is_write ? 
               (pass ? "写入成功" : "写入失败") : 
               (pass ? 
                   (strcmp(expected_str, actual_str) == 0 ? 
                       "值匹配" : "函数成功") : 
                   (strcmp(expected_str, actual_str) == 0 ? 
                       "函数失败" : "值不匹配")));
    
    if (!pass && !step->is_write) {
        printf("    期望: %s, 实际: %s\n", expected_str, actual_str);
    }
}

/**
 * @brief 运行单个测试用例
 * 
 * @param dm 设备管理器
 * @param test_case 测试用例
 * @return test_result_t 测试结果
 */
test_result_t run_test_case(device_manager_t* dm, const test_case_t* test_case) {
    if (!dm || !test_case) {
        printf("错误: 无效的参数\n");
        return TEST_RESULT_ERROR;
    }
    
    printf("\n===== 开始测试用例: %s =====\n", test_case->name);
    
    // 获取设备实例
    void* device = device_get(dm, test_case->device_type, test_case->device_id);
    if (!device) {
        printf("错误: 无法获取设备实例 (类型: %d, ID: %d)\n", 
               test_case->device_type, test_case->device_id);
        return TEST_RESULT_ERROR;
    }
    
    // 设置测试环境
    if (test_case->setup) {
        test_case->setup(dm, test_case->user_data);
    }
    
    int pass_count = 0;
    int total_steps = test_case->step_count;
    
    // 执行测试步骤
    for (int i = 0; i < test_case->step_count; i++) {
        const test_step_t* step = &test_case->steps[i];
        uint32_t actual_value = 0;
        bool pass = false;
        
        if (step->is_write) {
            // 写操作
            int result = device_write(device, step->reg_addr, step->write_value);
            pass = (result == 0);
            actual_value = step->write_value;
        } else {
            // 读操作
            int result = device_read(device, step->reg_addr, &actual_value);
            pass = (result == 0);
            
            // 如果读取成功，检查值
            if (pass) {
                if (step->format && strcmp(step->format, "float") == 0) {
                    // 浮点数比较
                    float expected = *((float*)&step->expected_value);
                    float actual = *((float*)&actual_value);
                    float scaled_expected = expected * step->value_scale;
                    float scaled_actual = actual * step->value_scale;
                    pass = (fabsf(scaled_expected - scaled_actual) < 0.001f);
                } else {
                    // 整数比较
                    pass = (actual_value == step->expected_value);
                }
            }
        }
        
        // 打印步骤结果
        print_test_step_result(step, actual_value, pass);
        
        if (pass) {
            pass_count++;
        }
    }
    
    // 清理测试环境
    if (test_case->cleanup) {
        test_case->cleanup(dm, test_case->user_data);
    }
    
    // 打印测试结果摘要
    printf("===== 测试用例结果: %d/%d 通过 =====\n", pass_count, total_steps);
    
    if (pass_count == total_steps) {
        return TEST_RESULT_PASS;
    } else {
        return TEST_RESULT_FAIL;
    }
}

/**
 * @brief 运行测试套件
 * 
 * @param dm 设备管理器
 * @param suite 测试套件
 * @return int 成功的测试用例数
 */
int run_test_suite(device_manager_t* dm, const test_suite_t* suite) {
    if (!dm || !suite) {
        printf("错误: 无效的参数\n");
        return -1;
    }
    
    printf("\n\n========== 开始测试套件: %s ==========\n", suite->name);
    
    int pass_count = 0;
    int total_cases = suite->test_case_count;
    
    for (int i = 0; i < suite->test_case_count; i++) {
        const test_case_t* test_case = suite->test_cases[i];
        test_result_t result = run_test_case(dm, test_case);
        
        if (result == TEST_RESULT_PASS) {
            pass_count++;
        }
    }
    
    printf("\n========== 测试套件结果: %d/%d 通过 ==========\n\n", pass_count, total_cases);
    
    return pass_count;
}

/**
 * @brief 初始化测试环境
 * 
 * @param dm 设备管理器
 * @param gm 全局监视器
 * @param am 动作管理器
 * @return int 成功返回0，失败返回负数
 */
int test_environment_init(device_manager_t** dm, global_monitor_t** gm, action_manager_t** am) {
    // 初始化动作管理器
    *am = action_manager_create();
    if (!*am) {
        printf("错误: 无法创建动作管理器\n");
        return -1;
    }
    
    // 初始化设备管理器
    *dm = device_manager_init();
    if (!*dm) {
        printf("错误: 无法创建设备管理器\n");
        action_manager_destroy(*am);
        *am = NULL;
        return -2;
    }
    
    // 初始化全局监视器
    *gm = global_monitor_create(*am, *dm);
    if (!*gm) {
        printf("错误: 无法创建全局监视器\n");
        device_manager_destroy(*dm);
        action_manager_destroy(*am);
        *dm = NULL;
        *am = NULL;
        return -3;
    }
    
    return 0;
}

/**
 * @brief 清理测试环境
 * 
 * @param dm 设备管理器
 * @param gm 全局监视器
 * @param am 动作管理器
 */
void test_environment_cleanup(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am) {
    if (gm) {
        global_monitor_destroy(gm);
    }
    
    if (dm) {
        device_manager_destroy(dm);
    }
    
    if (am) {
        action_manager_destroy(am);
    }
} 