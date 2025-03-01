/**
 * @file main.c
 * @brief 物理设备模拟器主程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
// 删除测试相关头文件
// #include "../../include/device_test.h"
#include "../../include/device_registry.h"
#include "../../include/device_types.h"
#include "../../include/global_monitor.h"
#include "../../include/action_manager.h"

// 核心组件头文件
#include "../../include/device_configs.h"
#include "../../include/device_rule_configs.h"

// 设备头文件（包含寄存器定义）
#include "../../plugins/flash/flash_device.h"
#include "../../plugins/fpga/fpga_device.h"
#include "../../plugins/temp_sensor/temp_sensor.h"

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
 * @brief 运行模拟器演示
 * 
 * @return int 成功返回0，失败返回非0
 */
static int run_simulator_demo(void) {
    printf("启动物理设备模拟器演示...\n");
    
    // 初始化管理器
    device_manager_t* dm = device_manager_init();
    
    if (!dm) {
        printf("错误: 无法初始化设备管理器\n");
        return 1;
    }
    
    // 使用正确的函数创建动作管理器
    action_manager_t* am = action_manager_create();
    
    if (!am) {
        printf("错误: 无法创建动作管理器\n");
        device_manager_destroy(dm);
        return 1;
    }
    
    // 使用动作管理器和设备管理器创建全局监视器
    global_monitor_t* gm = global_monitor_create(am, dm);
    
    if (!gm) {
        printf("错误: 无法创建全局监视器\n");
        device_manager_destroy(dm);
        action_manager_destroy(am);
        return 1;
    }
    
    // 注册设备类型
    register_all_device_types(dm);
    
    // 创建设备实例
    device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);
    device_instance_t* fpga = device_create(dm, DEVICE_TYPE_FPGA, 0);
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    
    if (!flash || !fpga || !temp_sensor) {
        printf("错误: 创建设备实例失败\n");
        // 清理资源
        device_manager_destroy(dm);
        global_monitor_destroy(gm);
        action_manager_destroy(am);
        return 1;
    }
    
    // 添加监视点
    global_monitor_add_watch(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_STATUS);
    global_monitor_add_watch(gm, DEVICE_TYPE_FPGA, 0, FPGA_STATUS_REG);
    global_monitor_add_watch(gm, DEVICE_TYPE_TEMP_SENSOR, 0, TEMP_REG);
    
    // 在这里可以添加演示代码
    // ...
    
    // 清理资源
    device_manager_destroy(dm);
    global_monitor_destroy(gm);
    action_manager_destroy(am);
    
    return 0;
}

/**
 * @brief 主函数
 * 
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 程序退出码
 */
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        // 运行测试模式
        extern int main(int argc, char* argv[]);
        char* test_args[] = {"test_main", "--all"};
        return main(2, test_args);
    } else {
        // 运行演示模式
        return run_simulator_demo();
    }
}