// device_tests.c
#include <stdio.h>
#include "device_tests.h"

// 运行所有设备测试
int run_all_device_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am) {
    printf("\n=== 开始运行所有设备测试 ===\n");
    
    int failed = 0;
    
    // 运行Flash设备测试
    if (run_flash_tests(dm, gm, am) != 0) {
        printf("Flash设备测试失败！\n");
        failed++;
    }
    
    // 运行温度传感器测试
    if (run_temp_sensor_tests(dm, gm, am) != 0) {
        printf("温度传感器测试失败！\n");
        failed++;
    }
    
    // 运行FPGA设备测试
    if (run_fpga_tests(dm, gm, am) != 0) {
        printf("FPGA设备测试失败！\n");
        failed++;
    }
    
    printf("\n=== 设备测试完成 ===\n");
    printf("总共测试: 3, 成功: %d, 失败: %d\n", 3 - failed, failed);
    
    return failed > 0 ? -1 : 0;
} 