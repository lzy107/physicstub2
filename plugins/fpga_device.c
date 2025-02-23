#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fpga_device.h"

// 私有函数声明
static int fpga_init(device_instance_t* instance);
static int fpga_read(device_instance_t* instance, uint32_t addr, uint32_t* value);
static int fpga_write(device_instance_t* instance, uint32_t addr, uint32_t value);
static void fpga_reset(device_instance_t* instance);
static void fpga_destroy(device_instance_t* instance);

// FPGA设备操作接口实现
static device_ops_t fpga_ops = {
    .init = fpga_init,
    .read = fpga_read,
    .write = fpga_write,
    .reset = fpga_reset,
    .destroy = fpga_destroy
};

// 获取FPGA设备操作接口
device_ops_t* get_fpga_device_ops(void) {
    return &fpga_ops;
}

// FPGA工作线程函数
static void* fpga_worker_thread(void* arg) {
    device_instance_t* instance = (device_instance_t*)arg;
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 检查FPGA是否使能
        if (dev_data->config_reg & CONFIG_ENABLE) {
            // 检查是否有操作请求
            if (dev_data->control_reg & CTRL_START) {
                // 设置忙状态
                dev_data->status_reg |= STATUS_BUSY;
                
                // 模拟FPGA操作
                usleep(100000);  // 模拟操作耗时100ms
                
                // 清除忙状态，设置完成状态
                dev_data->status_reg &= ~STATUS_BUSY;
                dev_data->status_reg |= STATUS_DONE;
                
                // 如果中断使能，触发中断
                if (dev_data->config_reg & CONFIG_IRQ_EN) {
                    dev_data->irq_reg |= 0x1;
                }
                
                // 清除启动位
                dev_data->control_reg &= ~CTRL_START;
            }
        }
        
        pthread_mutex_unlock(&dev_data->mutex);
        usleep(10000);  // 10ms轮询间隔
    }
    
    return NULL;
}

// 初始化FPGA设备
static int fpga_init(device_instance_t* instance) {
    if (!instance) return -1;
    
    // 分配设备私有数据
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)calloc(1, sizeof(fpga_dev_data_t));
    if (!dev_data) return -1;
    
    // 分配数据存储区
    dev_data->data_mem = (uint8_t*)calloc(FPGA_DATA_SIZE, sizeof(uint8_t));
    if (!dev_data->data_mem) {
        free(dev_data);
        return -1;
    }
    
    // 初始化互斥锁和寄存器
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->status_reg = STATUS_READY;  // 初始状态为就绪
    dev_data->config_reg = 0;
    dev_data->control_reg = 0;
    dev_data->irq_reg = 0;
    dev_data->running = 1;
    
    // 启动工作线程
    if (pthread_create(&dev_data->worker_thread, NULL, fpga_worker_thread, instance) != 0) {
        free(dev_data->data_mem);
        pthread_mutex_destroy(&dev_data->mutex);
        free(dev_data);
        return -1;
    }
    
    instance->private_data = dev_data;
    return 0;
}

// 读取FPGA寄存器或数据
static int fpga_read(device_instance_t* instance, uint32_t addr, uint32_t* value) {
    if (!instance || !value) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = 0;
    switch (addr) {
        case FPGA_STATUS_REG:
            *value = dev_data->status_reg;
            break;
        case FPGA_CONFIG_REG:
            *value = dev_data->config_reg;
            break;
        case FPGA_CONTROL_REG:
            *value = dev_data->control_reg;
            break;
        case FPGA_IRQ_REG:
            *value = dev_data->irq_reg;
            break;
        default:
            if (addr >= FPGA_DATA_START && 
                addr < (FPGA_DATA_START + FPGA_DATA_SIZE)) {
                uint32_t offset = addr - FPGA_DATA_START;
                *value = dev_data->data_mem[offset];
            } else {
                ret = -1;
            }
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 写入FPGA寄存器或数据
static int fpga_write(device_instance_t* instance, uint32_t addr, uint32_t value) {
    if (!instance) return -1;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return -1;
    
    pthread_mutex_lock(&dev_data->mutex);
    
    int ret = 0;
    switch (addr) {
        case FPGA_CONFIG_REG:
            dev_data->config_reg = value;
            // 如果设置了复位位，执行复位
            if (value & CONFIG_RESET) {
                fpga_reset(instance);
                dev_data->config_reg &= ~CONFIG_RESET;  // 清除复位位
            }
            break;
        case FPGA_CONTROL_REG:
            dev_data->control_reg = value;
            break;
        case FPGA_IRQ_REG:
            dev_data->irq_reg = value;
            break;
        default:
            if (addr >= FPGA_DATA_START && 
                addr < (FPGA_DATA_START + FPGA_DATA_SIZE)) {
                uint32_t offset = addr - FPGA_DATA_START;
                dev_data->data_mem[offset] = value & 0xFF;
            } else {
                ret = -1;
            }
    }
    
    pthread_mutex_unlock(&dev_data->mutex);
    return ret;
}

// 复位FPGA设备
static void fpga_reset(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 复位时不需要获取互斥锁，因为调用者已经持有锁
    
    // 复位所有寄存器
    dev_data->status_reg = STATUS_READY;
    dev_data->config_reg = 0;
    dev_data->control_reg = 0;
    dev_data->irq_reg = 0;
    
    // 清除数据存储区
    memset(dev_data->data_mem, 0, FPGA_DATA_SIZE);
}

// 销毁FPGA设备
static void fpga_destroy(device_instance_t* instance) {
    if (!instance) return;
    
    fpga_dev_data_t* dev_data = (fpga_dev_data_t*)instance->private_data;
    if (!dev_data) return;
    
    // 停止工作线程
    dev_data->running = 0;
    pthread_join(dev_data->worker_thread, NULL);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&dev_data->mutex);
    
    // 释放数据存储区
    if (dev_data->data_mem) {
        free(dev_data->data_mem);
        dev_data->data_mem = NULL;
    }
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
} 