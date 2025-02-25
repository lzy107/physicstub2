// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "device_registry.h"
#include "device_types.h"
#include "action_manager.h"
#include "global_monitor.h"
#include "device_test.h"
#include "../plugins/flash_device.h"
#include "../plugins/fpga_device.h"
#include "../plugins/temp_sensor.h"
#include "../plugins/device_tests.h"
#include "../plugins/test_rule_provider.h"

int main() {
    printf("初始化设备模拟器...\n");
    
    // 创建设备管理器
    device_manager_t* dm = device_manager_init();
    if (!dm) {
        printf("创建设备管理器失败\n");
        return 1;
    }
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("创建动作管理器失败\n");
        device_manager_destroy(dm);
        return 1;
    }
    
    // 创建全局监视器
    global_monitor_t* gm = global_monitor_create(am, dm);
    if (!gm) {
        printf("创建全局监视器失败\n");
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 注册测试规则提供者
    register_test_rule_provider();
    
    // 注册设备类型
    device_type_register(dm, DEVICE_TYPE_TEMP_SENSOR, "TEMP_SENSOR", get_temp_sensor_ops());
    device_type_register(dm, DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops());
    register_flash_device_type(dm);
    
    printf("设备注册表已初始化，包含 %d 种设备类型\n", MAX_DEVICE_TYPES);
    
    // 创建设备实例
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 0);
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);
    
    if (!temp_sensor || !fpga || !flash) {
        printf("创建设备实例失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 加载所有规则
    printf("\n加载动作规则...\n");
    if (action_manager_load_all_rules(am) != 0) {
        printf("加载动作规则失败\n");
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return 1;
    }
    
    // 运行所有设备测试
    if (run_all_device_tests(dm, gm, am) != 0) {
        printf("设备测试失败\n");
    } else {
        printf("所有设备测试通过\n");
    }
    
    // 清理资源
    printf("\n清理资源...\n");
    global_monitor_destroy(gm);
    action_manager_destroy(am);
    device_manager_destroy(dm);
    
    return 0;
}