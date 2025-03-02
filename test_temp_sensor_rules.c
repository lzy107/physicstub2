/**
 * @file test_temp_sensor_rules.c
 * @brief 温度传感器规则配置测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "device_registry.h"
#include "device_memory.h"
#include "action_manager.h"
#include "temp_sensor/temp_sensor.h"

// 超时设置
#define TEST_TIMEOUT_SECONDS 5
volatile int test_timeout_flag = 0;

// 超时处理函数
void test_timeout_handler(int sig) {
    printf("\n警告: 测试超时 (%d 秒)!\n", TEST_TIMEOUT_SECONDS);
    test_timeout_flag = 1;
}

// 测试温度传感器规则配置
int test_temp_sensor_rule_config() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 开始测试温度传感器规则配置...\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 创建设备管理器
    device_manager_t* dm = device_manager_init();
    if (!dm) {
        printf("[%ld.%06d] 创建设备管理器失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 设备管理器创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)dm);
    fflush(stdout);
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("[%ld.%06d] 创建动作管理器失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 动作管理器创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)am);
    fflush(stdout);
    
    // 注册温度传感器设备类型
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 开始注册温度传感器设备类型\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    register_temp_sensor_device_type(dm);
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 温度传感器设备类型注册成功\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 创建温度传感器设备实例
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 开始创建温度传感器设备实例\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    if (!temp_sensor) {
        printf("[%ld.%06d] 创建温度传感器设备实例失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 温度传感器设备实例创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)temp_sensor);
    fflush(stdout);
    
    // 获取温度传感器设备的操作函数
    device_type_t* temp_sensor_type = &dm->types[DEVICE_TYPE_TEMP_SENSOR];
    if (!temp_sensor_type) {
        printf("[%ld.%06d] 获取温度传感器设备类型失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 验证设备类型操作函数
    if (!temp_sensor_type->ops.read || !temp_sensor_type->ops.write) {
        printf("[%ld.%06d] 温度传感器设备类型没有有效的读写操作函数\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 验证设备可正常读写
    uint32_t test_value;
    if (temp_sensor_type->ops.read(temp_sensor, TEMP_REG, &test_value) != 0) {
        printf("[%ld.%06d] 测试读取温度传感器失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    printf("[%ld.%06d] 温度传感器初始值读取成功: 0x%08X (%.2f°C)\n", 
           tv.tv_sec, tv.tv_usec, test_value, test_value * 0.0625);
    fflush(stdout);
    
    // 获取温度传感器设备数据
    printf("获取温度传感器设备数据...\n");
    temp_sensor_device_t* ts_dev = (temp_sensor_device_t*)temp_sensor->priv_data;
    if (!ts_dev) {
        printf("[%ld.%06d] 获取温度传感器私有数据失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 打印温度传感器的规则信息
    printf("[%ld.%06d] 温度传感器规则数量: %d\n", tv.tv_sec, tv.tv_usec, ts_dev->rule_count);
    for (int i = 0; i < ts_dev->rule_count; i++) {
        device_rule_t* rule = &ts_dev->device_rules[i];
        printf("[%ld.%06d] 规则[%d]: 地址=0x%08X, 预期值=0x%08X, 掩码=0x%08X\n",
               tv.tv_sec, tv.tv_usec, i, rule->addr, rule->expected_value, rule->expected_mask);
        
        // 打印规则的目标动作
        if (rule->targets) {
            for (int j = 0; j < rule->targets->count; j++) {
                action_target_t* target = &rule->targets->targets[j];
                printf("[%ld.%06d]   目标[%d]: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X\n",
                       tv.tv_sec, tv.tv_usec, j, target->type, target->device_type, 
                       target->device_id, target->target_addr, target->target_value);
            }
        }
    }
    fflush(stdout);
    
    // 如果没有预定义规则，则测试失败
    if (ts_dev->rule_count == 0) {
        printf("[%ld.%06d] 错误: 温度传感器没有预定义规则，无法进行测试\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 使用第一个预定义规则进行测试
    device_rule_t* test_rule = &ts_dev->device_rules[0];
    
    // 测试触发规则
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 开始测试规则触发，写入值0x%08X到地址0x%08X\n", 
           tv.tv_sec, tv.tv_usec, test_rule->expected_value, test_rule->addr);
    
    // 读取触发前的值
    uint32_t before_value;
    if (temp_sensor_type->ops.read(temp_sensor, test_rule->addr, &before_value) != 0) {
        printf("[%ld.%06d] 读取触发前的值失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    printf("[%ld.%06d] 触发前地址0x%08X的值: 0x%08X\n", 
           tv.tv_sec, tv.tv_usec, test_rule->addr, before_value);
    
    // 写入触发值
    if (temp_sensor_type->ops.write(temp_sensor, test_rule->addr, test_rule->expected_value) != 0) {
        printf("[%ld.%06d] 写入触发值失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    // 获取目标地址和预期值
    uint32_t target_addr = 0;
    uint32_t expected_value = 0;
    
    if (test_rule->targets && test_rule->targets->count > 0) {
        action_target_t* target = &test_rule->targets->targets[0];
        target_addr = target->target_addr;
        expected_value = target->target_value;
    }
    
    // 读取结果
    uint32_t result_value;
    if (temp_sensor_type->ops.read(temp_sensor, target_addr, &result_value) != 0) {
        printf("[%ld.%06d] 读取结果值失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 规则触发后地址0x%08X的值: 0x%08X\n", 
           tv.tv_sec, tv.tv_usec, target_addr, result_value);
    
    // 验证结果
    if (result_value == expected_value) {
        printf("[%ld.%06d] 测试成功! 规则正确触发并执行了动作\n", tv.tv_sec, tv.tv_usec);
    } else {
        printf("[%ld.%06d] 测试失败! 规则未正确触发，预期值=0x%08X，实际值=0x%08X\n", 
               tv.tv_sec, tv.tv_usec, expected_value, result_value);
    }
    fflush(stdout);
    
    // 清理资源
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 清理资源...\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    device_destroy(dm, DEVICE_TYPE_TEMP_SENSOR, temp_sensor->dev_id);
    action_manager_destroy(am);
    device_manager_destroy(dm);
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06d] 资源清理完成\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    return 0;
}

int main(int argc, char** argv) {
    (void)argc; // 避免未使用参数警告
    (void)argv; // 避免未使用参数警告
    
    // 设置超时处理
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = test_timeout_handler;
    sigaction(SIGALRM, &sa, NULL);
    
    // 设置测试超时
    alarm(TEST_TIMEOUT_SECONDS);
    
    // 执行测试
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    
    printf("[%ld.%06d] 开始运行温度传感器规则测试...\n", 
           tv_start.tv_sec, tv_start.tv_usec);
    fflush(stdout);
    
    int ret = test_temp_sensor_rule_config();
    
    gettimeofday(&tv_end, NULL);
    long total_time_us = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + 
                         (tv_end.tv_usec - tv_start.tv_usec);
    
    printf("[%ld.%06d] 测试%s，耗时: %.2f 秒\n", 
           tv_end.tv_sec, tv_end.tv_usec, 
           ret == 0 ? "通过" : "失败", total_time_us / 1000000.0);
    
    // 停止超时定时器
    alarm(0);
    
    return ret;
} 