#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "device_manager.h"
#include "device_memory.h"
#include "action_manager.h"
#include "temp_sensor.h"

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
    printf("[%ld.%06ld] 开始测试温度传感器规则配置...\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 创建设备管理器
    device_manager_t* dm = device_manager_create();
    if (!dm) {
        printf("[%ld.%06ld] 创建设备管理器失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 设备管理器创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)dm);
    fflush(stdout);
    
    // 创建全局监视器（替换为NULL，因为已移除全局监视器）
    void* gm = NULL;
    
    // 创建动作管理器
    action_manager_t* am = action_manager_create();
    if (!am) {
        printf("[%ld.%06ld] 创建动作管理器失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 动作管理器创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)am);
    fflush(stdout);
    
    // 注册温度传感器设备类型
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 开始注册温度传感器设备类型\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    if (temp_sensor_register(dm) != 0) {
        printf("[%ld.%06ld] 注册温度传感器设备类型失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 温度传感器设备类型注册成功\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 创建温度传感器设备实例
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 开始创建温度传感器设备实例\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    device_instance_t* temp_sensor = device_create(dm, DEVICE_TYPE_TEMP_SENSOR, 0);
    if (!temp_sensor) {
        printf("[%ld.%06ld] 创建温度传感器设备实例失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 温度传感器设备实例创建成功: %p\n", tv.tv_sec, tv.tv_usec, (void*)temp_sensor);
    fflush(stdout);
    
    // 获取温度传感器设备内存
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 开始获取温度传感器设备内存\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    device_memory_t* memory = temp_sensor_get_memory(temp_sensor);
    if (!memory) {
        printf("[%ld.%06ld] 获取温度传感器设备内存失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        device_destroy(dm, temp_sensor);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 温度传感器设备内存获取成功: %p, 区域数量: %d\n", 
           tv.tv_sec, tv.tv_usec, (void*)memory, memory->region_count);
    fflush(stdout);
    
    // 打印内存区域信息
    for (int i = 0; i < memory->region_count; i++) {
        printf("[%ld.%06ld] 内存区域 #%d: 基址=0x%08X, 单位大小=%u, 长度=%u\n", 
               tv.tv_sec, tv.tv_usec, i, memory->regions[i].base_addr, 
               memory->regions[i].unit_size, memory->regions[i].length);
        fflush(stdout);
    }
    
    // 添加更多调试输出，查看内存当前的内容
    printf("[%ld.%06ld] 检查温度传感器设备内存初始状态:\n", tv.tv_sec, tv.tv_usec);
    
    // 检查温度传感器设备私有数据
    temp_sensor_device_t* ts_dev = (temp_sensor_device_t*)temp_sensor->priv_data;
    printf("[%ld.%06ld] 温度传感器设备指针: %p\n", tv.tv_sec, tv.tv_usec, (void*)temp_sensor);
    printf("[%ld.%06ld] 温度传感器私有数据指针: %p\n", tv.tv_sec, tv.tv_usec, (void*)ts_dev);
    printf("[%ld.%06ld] 温度传感器内存指针: %p\n", tv.tv_sec, tv.tv_usec, ts_dev ? (void*)ts_dev->memory : NULL);
    
    if (ts_dev && ts_dev->memory) {
        printf("[%ld.%06ld] 温度传感器内存区域数量: %d\n", tv.tv_sec, tv.tv_usec, ts_dev->memory->region_count);
        for (int i = 0; i < ts_dev->memory->region_count; i++) {
            printf("[%ld.%06ld] 内存区域 #%d: 基址=0x%08X, 数据指针=%p\n", 
                   tv.tv_sec, tv.tv_usec, i, ts_dev->memory->regions[i].base_addr, 
                   ts_dev->memory->regions[i].data);
        }
    }
    
    for (uint32_t addr = 0; addr < 16; addr += 4) {
        uint32_t value = 0;
        int read_ret = device_read(temp_sensor, addr, &value);
        printf("[%ld.%06ld] 地址 0x%08X: 0x%08X (读取结果: %d)\n", 
               tv.tv_sec, tv.tv_usec, addr, read_ret == 0 ? value : 0, read_ret);
    }
    fflush(stdout);
    
    // 检查温度传感器内存的有效性
    printf("[%ld.%06ld] 检查温度传感器内存是否有效:\n", tv.tv_sec, tv.tv_usec);
    printf("[%ld.%06ld] memory->regions[0].data: %p\n", tv.tv_sec, tv.tv_usec, memory->regions[0].data);
    
    // 检查寄存器地址
    printf("[%ld.%06ld] 温度寄存器地址: TEMP_REG=0x%02X, CONFIG_REG=0x%02X\n", 
           tv.tv_sec, tv.tv_usec, TEMP_REG, CONFIG_REG);
    fflush(stdout);
    
    // 构建动作目标
    action_target_t target;
    action_target_array_t targets;
    memset(&target, 0, sizeof(target));
    memset(&targets, 0, sizeof(targets));
    
    // 配置目标 - 使用在init中已初始化的寄存器地址
    target.device_type = DEVICE_TYPE_TEMP_SENSOR;
    target.device_id = 0;
    target.target_addr = 0x0;  // 修改为温度寄存器地址
    target.target_value = 0x5;
    target.type = ACTION_TYPE_WRITE;  // 添加动作类型
    target.target_mask = 0xFFFFFFFF;  // 添加掩码
    
    // 添加到目标数组
    action_target_array_init(&targets);
    action_target_array_add(&targets, &target);
    
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 配置规则: 当温度传感器(ID=0)地址0x%X的值为0x%X时，将地址0x%X的值设置为0x%X\n",
           tv.tv_sec, tv.tv_usec, 0x0, 0x3, 0x0, 0x5);
    fflush(stdout);

    // 直接添加温度传感器规则 - 使用已知有效的寄存器地址
    int rule_id = temp_sensor_add_rule(temp_sensor, 0x0, 0x3, 0xF, &targets);
    gettimeofday(&tv, NULL);
    if (rule_id < 0) {
        printf("[%ld.%06ld] 设置温度传感器规则失败，返回值=%d\n", tv.tv_sec, tv.tv_usec, rule_id);
        fflush(stdout);
        device_destroy(dm, temp_sensor);
        action_manager_destroy(am);
        device_manager_destroy(dm);
        return -1;
    }
    
    printf("[%ld.%06ld] 温度传感器规则设置成功，规则ID=%d\n", tv.tv_sec, tv.tv_usec, rule_id);
    printf("[%ld.%06ld] 成功加载规则: 当地址0x0的值为0x3时，将地址0x0的值设置为0x5\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 打印动作管理器的状态
    if (am) {
        printf("[%ld.%06ld] 规则创建后动作管理器状态: rule_count=%d\n", 
               tv.tv_sec, tv.tv_usec, am->rule_count);
        
        // 打印所有规则的信息
        for (int i = 0; i < am->rule_count; i++) {
            action_rule_t* rule = &am->rules[i];
            printf("[%ld.%06ld] 规则 #%d: ID=%d, 名称=%s, 目标数量=%d\n", 
                   tv.tv_sec, tv.tv_usec, i, rule->rule_id, 
                   rule->name ? rule->name : "未命名", rule->targets.count);
            
            // 打印规则的目标信息
            for (int j = 0; j < rule->targets.count; j++) {
                action_target_t* t = &rule->targets.targets[j];
                printf("[%ld.%06ld]   目标 #%d: 类型=%d, 设备类型=%d, 设备ID=%d, 地址=0x%08X, 值=0x%08X\n", 
                       tv.tv_sec, tv.tv_usec, j, t->type, t->device_type, t->device_id, 
                       t->target_addr, t->target_value);
            }
        }
    } else {
        printf("[%ld.%06ld] 警告: 动作管理器为NULL\n", tv.tv_sec, tv.tv_usec);
    }
    fflush(stdout);
    
    // 测试触发条件
    struct timeval write_start, write_end;
    gettimeofday(&write_start, NULL);
    printf("[%ld.%06ld] 开始写入触发值\n", write_start.tv_sec, write_start.tv_usec);
    fflush(stdout);

    // 检查温度传感器内存的有效性
    ts_dev = (temp_sensor_device_t*)temp_sensor->priv_data;
    if (!ts_dev) {
        printf("[%ld.%06ld] 错误: 温度传感器私有数据为NULL\n", write_start.tv_sec, write_start.tv_usec);
        goto cleanup;
    }

    if (!ts_dev->memory) {
        printf("[%ld.%06ld] 错误: 温度传感器内存为NULL\n", write_start.tv_sec, write_start.tv_usec);
        goto cleanup;
    }

    // 详细打印内存区域信息
    printf("[%ld.%06ld] 内存区域检查: 区域数量=%d\n", write_start.tv_sec, write_start.tv_usec, ts_dev->memory->region_count);
    if (ts_dev->memory->region_count > 0) {
        for (int i = 0; i < ts_dev->memory->region_count; i++) {
            printf("[%ld.%06ld] 区域[%d]: 基址=0x%08X, 单位大小=%zu, 长度=%zu, 数据指针=%p\n",
                   write_start.tv_sec, write_start.tv_usec, i,
                   ts_dev->memory->regions[i].base_addr,
                   ts_dev->memory->regions[i].unit_size,
                   ts_dev->memory->regions[i].length,
                   ts_dev->memory->regions[i].data);
            
            // 尝试直接从内存读取，而不是通过device_read
            if (ts_dev->memory->regions[i].data) {
                printf("[%ld.%06ld] 尝试直接从内存读取区域[%d]的前几个值:\n", 
                       write_start.tv_sec, write_start.tv_usec, i);
                
                for (uint32_t offset = 0; offset < 16; offset += 4) {
                    uint32_t value = 0;
                    uint8_t* ptr = ts_dev->memory->regions[i].data + offset;
                    memcpy(&value, ptr, sizeof(uint32_t));
                    printf("[%ld.%06ld]   偏移 0x%02X (地址 0x%08X): 0x%08X\n", 
                           write_start.tv_sec, write_start.tv_usec, 
                           offset, ts_dev->memory->regions[i].base_addr + offset, value);
                }
            }
        }
    }

    // 尝试直接使用内存函数而不是设备函数
    uint32_t test_value = 0;
    int mem_read_result = device_memory_read(ts_dev->memory, 0x0, &test_value);
    printf("[%ld.%06ld] 直接内存读取结果: 地址=0x0, 值=0x%08X, 返回=%d\n", 
           write_start.tv_sec, write_start.tv_usec, test_value, mem_read_result);
    
    // 写入测试值
    int mem_write_result = device_memory_write(ts_dev->memory, 0x0, 0x3);
    printf("[%ld.%06ld] 直接内存写入结果: 地址=0x0, 值=0x3, 返回=%d\n", 
           write_start.tv_sec, write_start.tv_usec, mem_write_result);
    
    // 重新读取确认写入
    if (mem_write_result == 0) {
        test_value = 0;
        mem_read_result = device_memory_read(ts_dev->memory, 0x0, &test_value);
        printf("[%ld.%06ld] 写入后直接内存读取结果: 地址=0x0, 值=0x%08X, 返回=%d\n", 
               write_start.tv_sec, write_start.tv_usec, test_value, mem_read_result);
    }

    // 获取写入前的寄存器状态
    uint32_t before_trigger_value = 0;
    int read_result = device_read(temp_sensor, 0x0, &before_trigger_value);
    gettimeofday(&tv, NULL);
    if (read_result != 0) {
        printf("[%ld.%06ld] 读取目标寄存器(地址=0x0)的初始值失败: %d\n", 
               tv.tv_sec, tv.tv_usec, read_result);
    } else {
        printf("[%ld.%06ld] 触发前目标寄存器(地址=0x0)的值: 0x%08X\n", 
               tv.tv_sec, tv.tv_usec, before_trigger_value);
    }
    fflush(stdout);
    
    // 写入触发值
    int write_result = device_write(temp_sensor, 0x0, 0x3);
    gettimeofday(&write_end, NULL);
    
    // 计算写入操作的耗时
    long write_time_us = (write_end.tv_sec - write_start.tv_sec) * 1000000 + 
                          (write_end.tv_usec - write_start.tv_usec);
    
    // 检查写入结果
    if (write_result != 0) {
        printf("[%ld.%06ld] 写入触发值(地址=0x0, 值=0x3)失败: %d\n", 
               write_end.tv_sec, write_end.tv_usec, write_result);
        goto cleanup;
    } else {
        printf("[%ld.%06ld] 写入触发值(地址=0x0, 值=0x3)成功，耗时: %ld 微秒\n", 
               write_end.tv_sec, write_end.tv_usec, write_time_us);
    }
    
    // 延迟一段时间，等待动作生效
    usleep(10000);  // 休眠10毫秒
    
    // 检查动作是否已触发
    uint32_t after_value = 0;
    if (device_read(temp_sensor, 0x0, &after_value) != 0) {
        printf("[%ld.%06ld] 读取目标寄存器(地址=0x0)的最终值失败\n", 
               write_end.tv_sec, write_end.tv_usec);
        goto cleanup;
    }
    
    // 验证是否值被修改为预期的值
    if (after_value == 0x5) {
        printf("[%ld.%06ld] 测试成功! 目标寄存器值已被修改为0x%08X\n", 
               write_end.tv_sec, write_end.tv_usec, after_value);
    } else {
        printf("[%ld.%06ld] 测试失败! 目标寄存器值未被修改，当前值为: 0x%08X\n", 
               write_end.tv_sec, write_end.tv_usec, after_value);
        goto cleanup;
    }
    
cleanup:
    // 清理资源
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 开始清理资源\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 销毁温度传感器设备
    if (temp_sensor) {
        device_destroy(dm, temp_sensor);
    }
    
    // 销毁动作管理器
    if (am) {
        action_manager_destroy(am);
    }
    
    // 销毁设备管理器
    if (dm) {
        device_manager_destroy(dm);
    }
    
    // 计算测试总耗时
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    double elapsed = (end_time.tv_sec - tv.tv_sec) + 
                    (end_time.tv_usec - tv.tv_usec) / 1000000.0;
    
    if (write_result == 0 && after_value == 0x5) {
        printf("[%ld.%06ld] 测试成功，耗时: %.2f 秒\n", 
               end_time.tv_sec, end_time.tv_usec, elapsed);
        return 0;
    } else {
        printf("[%ld.%06ld] 测试失败，耗时: %.2f 秒\n", 
               end_time.tv_sec, end_time.tv_usec, elapsed);
        return -1;
    }
}

int main() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 温度传感器规则测试开始\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 设置超时处理
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = test_timeout_handler;
    sigaction(SIGALRM, &sa, NULL);
    
    // 设置超时时间
    alarm(TEST_TIMEOUT_SECONDS);
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 超时设置为 %d 秒\n", tv.tv_sec, tv.tv_usec, TEST_TIMEOUT_SECONDS);
    fflush(stdout);
    
    // 运行测试
    int result = test_temp_sensor_rule_config();
    
    // 取消超时
    alarm(0);
    gettimeofday(&tv, NULL);
    printf("[%ld.%06ld] 测试完成，取消超时\n", tv.tv_sec, tv.tv_usec);
    fflush(stdout);
    
    // 根据测试结果和超时状态设置返回值
    if (test_timeout_flag) {
        printf("[%ld.%06ld] 测试结果: 超时\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        return 2;  // 超时返回特殊值
    } else if (result == 0) {
        printf("[%ld.%06ld] 测试结果: 通过\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        return 0;
    } else {
        printf("[%ld.%06ld] 测试结果: 失败\n", tv.tv_sec, tv.tv_usec);
        fflush(stdout);
        return 1;
    }
} 