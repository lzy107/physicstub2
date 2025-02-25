// device_tests.h
#ifndef DEVICE_TESTS_H
#define DEVICE_TESTS_H

#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/global_monitor.h"

// 温度传感器测试
int run_temp_sensor_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am);

// FPGA设备测试
int run_fpga_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am);

// Flash设备测试
int run_flash_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am);

// 运行所有设备测试
int run_all_device_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am);

#endif // DEVICE_TESTS_H 