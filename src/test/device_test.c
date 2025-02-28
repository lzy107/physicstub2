#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "device_test.h"

// 运行单个测试步骤
static int run_test_step(device_instance_t* instance, device_ops_t* ops, const test_step_t* step) {
    if (!instance || !ops || !step) return -1;
    
    uint32_t value = 0;
    char value_str[32];  // 用于格式化输出的缓冲区
    
    printf("  %s...\n", step->name);
    
    if (step->is_write) {
        // 执行写操作
        if (!ops->write || ops->write(instance, step->reg_addr, step->write_value) != 0) {
            printf("    Failed!\n");
            return -1;
        }
        
        // 格式化输出写入的值
        if (step->format) {
            if (step->value_scale != 0.0f) {
                snprintf(value_str, sizeof(value_str), step->format, 
                    step->write_value * step->value_scale);
            } else {
                snprintf(value_str, sizeof(value_str), step->format, 
                    step->write_value);
            }
            printf("    Wrote: %s\n", value_str);
        } else {
            printf("    Wrote value successfully\n");
        }
    } else {
        // 执行读操作
        if (!ops->read || ops->read(instance, step->reg_addr, &value) != 0) {
            printf("    Failed!\n");
            return -1;
        }
        
        // 如果指定了期望值，进行验证
        if (step->expect_value != 0 && value != step->expect_value) {
            printf("    Failed! Expected %u, got %u\n", step->expect_value, value);
            return -1;
        }
        
        // 打印读取的值
        if (step->format) {
            if (step->value_scale != 0.0f) {
                snprintf(value_str, sizeof(value_str), step->format, 
                    value * step->value_scale);
            } else {
                snprintf(value_str, sizeof(value_str), step->format, value);
            }
            printf("    Read: %s\n", value_str);
        } else {
            printf("    Read successfully\n");
        }
    }
    
    return 0;
}

int run_test_case(device_manager_t* dm, const test_case_t* test_case) {
    if (!dm || !test_case) return -1;
    
    printf("\nRunning test case: %s\n", test_case->name);
    
    // 获取已存在的设备实例
    device_instance_t* instance = device_get(dm, test_case->device_type, test_case->device_id);
    if (!instance) {
        printf("Failed to get device instance\n");
        return -1;
    }
    
    // 获取设备操作接口
    device_ops_t ops = dm->types[test_case->device_type].ops;
    
    // 执行设置函数（如果有）
    if (test_case->setup) {
        test_case->setup(instance);
    }
    
    // 执行测试步骤
    for (int i = 0; i < test_case->step_count; i++) {
        if (run_test_step(instance, &ops, &test_case->steps[i]) != 0) {
            printf("Test case failed at step %d\n", i + 1);
            if (test_case->cleanup) {
                test_case->cleanup(instance);
            }
            return -1;
        }
        
        // 如果不是最后一步，等待一小段时间
        if (i < test_case->step_count - 1) {
            usleep(100000);  // 等待100ms
        }
    }
    
    // 执行清理函数（如果有）
    if (test_case->cleanup) {
        test_case->cleanup(instance);
    }
    
    printf("Test case completed successfully!\n");
    return 0;
}

int run_test_suite(device_manager_t* dm, const test_case_t* test_cases, int case_count) {
    if (!dm || !test_cases || case_count <= 0) return -1;
    
    printf("\nRunning test suite with %d test cases...\n", case_count);
    int failed_count = 0;
    
    for (int i = 0; i < case_count; i++) {
        if (run_test_case(dm, &test_cases[i]) != 0) {
            failed_count++;
        }
    }
    
    printf("\nTest suite completed. %d passed, %d failed.\n", 
        case_count - failed_count, failed_count);
    
    return failed_count == 0 ? 0 : -1;
} 