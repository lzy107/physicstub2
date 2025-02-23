#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fpga_device.h"
#include "device_registry.h"

// 注册FPGA设备
REGISTER_DEVICE(DEVICE_TYPE_FPGA, "FPGA", get_fpga_device_ops);

// FPGA设备内存区域配置
static device_mem_region_t fpga_regions[FPGA_REGION_COUNT] = {
    // 寄存器区域
    {
        .start_addr = FPGA_STATUS_REG,
        .unit_size = sizeof(uint32_t),
        .unit_count = 4,  // 4个32位寄存器
        .mem_ptr = NULL
    },
    // 数据区域
    {
        .start_addr = FPGA_DATA_START,
        .unit_size = sizeof(uint8_t),
        .unit_count = 0x1000,  // 4KB数据区
        .mem_ptr = NULL
    }
};

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
    uint32_t value;
    
    while (dev_data->running) {
        pthread_mutex_lock(&dev_data->mutex);
        
        // 读取配置寄存器
        if (device_mem_read(&dev_data->mem_config, FPGA_CONFIG_REG, &value, sizeof(value)) == 0) {
            // 检查FPGA是否使能
            if (value & CONFIG_ENABLE) {
                // 读取控制寄存器
                if (device_mem_read(&dev_data->mem_config, FPGA_CONTROL_REG, &value, sizeof(value)) == 0) {
                    // 检查是否有操作请求
                    if (value & CTRL_START) {
                        // 设置忙状态
                        uint32_t status = STATUS_BUSY;
                        device_mem_write(&dev_data->mem_config, FPGA_STATUS_REG, &status, sizeof(status));
                        
                        // 模拟FPGA操作
                        usleep(100000);  // 模拟操作耗时100ms
                        
                        // 清除忙状态，设置完成状态
                        status = STATUS_DONE;
                        device_mem_write(&dev_data->mem_config, FPGA_STATUS_REG, &status, sizeof(status));
                        
                        // 如果中断使能，触发中断
                        if (device_mem_read(&dev_data->mem_config, FPGA_CONFIG_REG, &value, sizeof(value)) == 0) {
                            if (value & CONFIG_IRQ_EN) {
                                uint32_t irq = 0x1;
                                device_mem_write(&dev_data->mem_config, FPGA_IRQ_REG, &irq, sizeof(irq));
                            }
                        }
                        
                        // 清除启动位
                        value &= ~CTRL_START;
                        device_mem_write(&dev_data->mem_config, FPGA_CONTROL_REG, &value, sizeof(value));
                    }
                }
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
    
    // 初始化内存配置
    dev_data->mem_config.regions = fpga_regions;
    dev_data->mem_config.region_count = FPGA_REGION_COUNT;
    
    // 初始化设备内存
    if (device_mem_init(&dev_data->mem_config) != 0) {
        free(dev_data);
        return -1;
    }
    
    // 初始化互斥锁
    pthread_mutex_init(&dev_data->mutex, NULL);
    dev_data->running = 1;
    
    // 设置初始状态
    uint32_t status = STATUS_READY;
    device_mem_write(&dev_data->mem_config, FPGA_STATUS_REG, &status, sizeof(status));
    
    // 启动工作线程
    if (pthread_create(&dev_data->worker_thread, NULL, fpga_worker_thread, instance) != 0) {
        device_mem_destroy(&dev_data->mem_config);
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
    int ret = device_mem_read(&dev_data->mem_config, addr, value, 
        addr < FPGA_DATA_START ? sizeof(uint32_t) : sizeof(uint8_t));
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
    if (addr == FPGA_CONFIG_REG) {
        // 如果设置了复位位，执行复位
        if (value & CONFIG_RESET) {
            fpga_reset(instance);
            value &= ~CONFIG_RESET;  // 清除复位位
        }
    }
    
    ret = device_mem_write(&dev_data->mem_config, addr, &value,
        addr < FPGA_DATA_START ? sizeof(uint32_t) : sizeof(uint8_t));
    
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
    uint32_t zero = 0;
    device_mem_write(&dev_data->mem_config, FPGA_CONFIG_REG, &zero, sizeof(zero));
    device_mem_write(&dev_data->mem_config, FPGA_CONTROL_REG, &zero, sizeof(zero));
    device_mem_write(&dev_data->mem_config, FPGA_IRQ_REG, &zero, sizeof(zero));
    
    uint32_t status = STATUS_READY;
    device_mem_write(&dev_data->mem_config, FPGA_STATUS_REG, &status, sizeof(status));
    
    // 清除数据区
    uint8_t* data_region = (uint8_t*)dev_data->mem_config.regions[FPGA_DATA_REGION].mem_ptr;
    memset(data_region, 0, dev_data->mem_config.regions[FPGA_DATA_REGION].unit_count);
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
    
    // 释放设备内存
    device_mem_destroy(&dev_data->mem_config);
    
    // 释放设备数据
    free(dev_data);
    instance->private_data = NULL;
} 