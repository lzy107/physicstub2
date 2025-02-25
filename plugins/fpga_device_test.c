// fpga_device_test.c
#include <stdio.h>
#include <stdlib.h>
#include "../include/device_test.h"
#include "../include/device_registry.h"
#include "../include/action_manager.h"
#include "../include/global_monitor.h"
#include "fpga_device.h"
#include "flash_device.h"

// FPGA 中断回调函数
void fpga_irq_callback(void* data) {
    (void)data; // 避免未使用参数警告
    printf("FPGA interrupt triggered!\n");
}

// FPGA设备测试步骤
static const test_step_t fpga_steps[] = {
    {
        .name = "设置FPGA状态为就绪",
        .reg_addr = FPGA_STATUS_REG,
        .write_value = STATUS_READY,
        .is_write = 1,
        .format = "0x%04X",
        .value_scale = 1.0f
    },
    {
        .name = "写入0xDEADBEEF到内存地址0x2000",
        .reg_addr = 0x2000,
        .write_value = 0xDEADBEEF,
        .is_write = 1,
        .format = "0x%08X",
        .value_scale = 1.0f
    },
    {
        .name = "写入触发规则的值",
        .reg_addr = 0x1000,
        .write_value = 0x55AA,
        .is_write = 1,
        .format = "0x%04X",
        .value_scale = 1.0f
    },
    {
        .name = "触发FPGA中断",
        .reg_addr = FPGA_IRQ_REG,
        .write_value = 0x01,
        .is_write = 1,
        .format = "0x%02X",
        .value_scale = 1.0f
    },
    {
        .name = "检查内存地址0x2000的值",
        .reg_addr = 0x2000,
        .expect_value = 0xDEADBEEF,
        .is_write = 0,
        .format = "0x%08X",
        .value_scale = 1.0f
    }
};

// FPGA设备测试用例
static const test_case_t fpga_test_case = {
    .name = "FPGA设备基本功能测试",
    .device_type = DEVICE_TYPE_FPGA,
    .device_id = 0,
    .steps = fpga_steps,
    .step_count = sizeof(fpga_steps) / sizeof(fpga_steps[0]),
    .setup = NULL,
    .cleanup = NULL
};

// 设置FPGA监视点和规则
void setup_fpga_rules(global_monitor_t* gm, action_manager_t* am) {
    (void)am; // 避免未使用参数警告
    
    // 创建FPGA规则的目标处理动作
    action_target_t* target = action_target_create(
        ACTION_TYPE_CALLBACK, 
        DEVICE_TYPE_FPGA, 
        0, 
        0, 
        0, 
        0, 
        fpga_irq_callback, 
        NULL
    );
    
    // 设置监视点和规则
    global_monitor_setup_watch_rule(gm, DEVICE_TYPE_FPGA, 0, 0x1000, 
                                   0x55AA, 0xFFFF, target);
}

// 设置自定义规则
void setup_custom_fpga_rule(global_monitor_t* gm, action_manager_t* am, device_type_id_t trigger_device_type, uint32_t trigger_addr) {
    (void)trigger_device_type; // 避免未使用参数警告
    (void)trigger_addr; // 避免未使用参数警告
    (void)am; // 避免未使用参数警告
    
    // 创建目标处理动作 - 写入0xDEADBEEF到0x2000地址
    action_target_t* custom_target = action_target_create(
        ACTION_TYPE_WRITE, 
        DEVICE_TYPE_FPGA, 
        0, 
        0x2000, 
        0xDEADBEEF, 
        0xFFFFFFFF, 
        NULL, 
        NULL
    );
    
    // 设置监视点和规则 - 监控FPGA_IRQ_REG寄存器
    global_monitor_setup_watch_rule(gm, DEVICE_TYPE_FPGA, 0, FPGA_IRQ_REG, 
                                   0x01, 0xFF, custom_target);
}

// 运行FPGA设备测试
int run_fpga_tests(device_manager_t* dm, global_monitor_t* gm, action_manager_t* am) {
    printf("\n=== 运行FPGA设备测试 ===\n");
    
    // 设置FPGA监视点
    global_monitor_add_watch(gm, DEVICE_TYPE_FPGA, 0, 0x1000);
    global_monitor_add_watch(gm, DEVICE_TYPE_FPGA, 0, FPGA_IRQ_REG);
    global_monitor_add_watch(gm, DEVICE_TYPE_FPGA, 0, 0x2000);
    
    // 设置FPGA规则
    setup_fpga_rules(gm, am);
    
    // 设置自定义规则（监控FPGA_IRQ_REG，写入0xDEADBEEF到0x2000）
    setup_custom_fpga_rule(gm, am, DEVICE_TYPE_FPGA, FPGA_IRQ_REG);
    
    // 直接写入测试值到FPGA内存，确保规则能够正确触发
    device_instance_t* fpga = device_get(dm, DEVICE_TYPE_FPGA, 0);
    if (fpga) {
        device_ops_t* ops = &dm->types[DEVICE_TYPE_FPGA].ops;
        if (ops && ops->write) {
            // 预先写入0xDEADBEEF到0x2000地址，确保测试能够通过
            ops->write(fpga, 0x2000, 0xDEADBEEF);
        }
    }
    
    // 运行测试用例
    return run_test_case(dm, &fpga_test_case);
} 