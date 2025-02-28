/**
 * @file test_main.c
 * @brief 测试主程序入口
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

// 设备头文件（包含寄存器定义）
#include "../../plugins/flash/flash_device.h"
#include "../../plugins/fpga/fpga_device.h"
#include "../../plugins/temp_sensor/temp_sensor.h"

// 前向声明设备测试函数
extern test_suite_t* create_flash_test_suite(void);
extern test_suite_t* create_fpga_test_suite(void);
extern test_suite_t* create_temp_sensor_test_suite(void);
extern void destroy_test_suite(test_suite_t* suite);

/**
 * @brief 打印使用说明
 */
static void print_usage(const char* program_name) {
    printf("使用方法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -h, --help            显示此帮助信息\n");
    printf("  -a, --all             运行所有测试用例\n");
    printf("  -f, --flash           运行Flash设备测试\n");
    printf("  -g, --fpga            运行FPGA设备测试\n");
    printf("  -t, --temp-sensor     运行温度传感器测试\n");
    printf("  -v, --verbose         显示详细输出\n");
}

/**
 * @brief 注册所有设备类型
 * 
 * @param dm 设备管理器
 */
static void register_all_device_types(device_manager_t* dm) {
    // 这里假设有外部函数用于获取设备操作接口
    extern device_ops_t* get_flash_device_ops(void);
    extern device_ops_t* get_fpga_device_ops(void);
    extern device_ops_t* get_temp_sensor_ops(void);
    
    // 注册Flash设备类型
    device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", get_flash_device_ops());
    
    // 注册FPGA设备类型
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops());
    
    // 注册温度传感器设备类型
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops());
}

/**
 * @brief 测试主程序入口
 * 
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 程序退出码
 */
int main(int argc, char* argv[]) {
    bool run_all = false;
    bool run_flash = false;
    bool run_fpga = false;
    bool run_temp_sensor = false;
    bool verbose = false;
    
    // 解析命令行参数
    if (argc == 1) {
        // 默认运行所有测试
        run_all = true;
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_usage(argv[0]);
                return 0;
            } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
                run_all = true;
            } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--flash") == 0) {
                run_flash = true;
            } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--fpga") == 0) {
                run_fpga = true;
            } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--temp-sensor") == 0) {
                run_temp_sensor = true;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
                verbose = true;
            } else {
                printf("错误: 未知参数 '%s'\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        }
    }
    
    // 初始化测试环境
    device_manager_t* dm = NULL;
    global_monitor_t* gm = NULL;
    action_manager_t* am = NULL;
    
    if (test_environment_init(&dm, &gm, &am) != 0) {
        printf("错误: 无法初始化测试环境\n");
        return 1;
    }
    
    // 注册设备类型
    register_all_device_types(dm);
    
    int total_pass = 0;
    int total_suites = 0;
    
    printf("\n======================================================\n");
    printf("        设备模拟器测试框架 v1.0\n");
    printf("======================================================\n\n");
    
    // 运行Flash设备测试
    if (run_all || run_flash) {
        test_suite_t* flash_suite = create_flash_test_suite();
        if (flash_suite) {
            total_pass += run_test_suite(dm, flash_suite);
            total_suites += flash_suite->test_case_count;
            destroy_test_suite(flash_suite);
        }
    }
    
    // 运行FPGA设备测试
    if (run_all || run_fpga) {
        test_suite_t* fpga_suite = create_fpga_test_suite();
        if (fpga_suite) {
            total_pass += run_test_suite(dm, fpga_suite);
            total_suites += fpga_suite->test_case_count;
            destroy_test_suite(fpga_suite);
        }
    }
    
    // 运行温度传感器测试
    if (run_all || run_temp_sensor) {
        test_suite_t* temp_sensor_suite = create_temp_sensor_test_suite();
        if (temp_sensor_suite) {
            total_pass += run_test_suite(dm, temp_sensor_suite);
            total_suites += temp_sensor_suite->test_case_count;
            destroy_test_suite(temp_sensor_suite);
        }
    }
    
    // 打印总体测试结果
    printf("\n======================================================\n");
    printf("测试执行完成: %d/%d 测试用例通过\n", total_pass, total_suites);
    printf("======================================================\n\n");
    
    // 清理测试环境
    test_environment_cleanup(dm, gm, am);
    
    return (total_pass == total_suites) ? 0 : 1;
} 