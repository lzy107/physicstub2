#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>
#include <pthread.h>
#include "device_types.h"
#include "device_memory.h"

// 设备类型ID
#define DEVICE_TYPE_TEMP_SENSOR 1

// 寄存器地址定义
#define TEMP_REG         0x00    // 温度寄存器 (R)
#define CONFIG_REG       0x01    // 配置寄存器 (R/W)
#define TLOW_REG        0x02    // 低温报警寄存器 (R/W)
#define THIGH_REG       0x03    // 高温报警寄存器 (R/W)

// 配置寄存器位定义
#define CONFIG_SHUTDOWN  (1 << 0)  // 关断模式
#define CONFIG_ALERT    (1 << 1)   // 报警输出使能
#define CONFIG_POLARITY (1 << 2)   // 报警极性
#define CONFIG_FQUEUE   (3 << 3)   // 故障队列
#define CONFIG_RES     (3 << 5)    // 转换分辨率
#define CONFIG_ONESHOT (1 << 7)    // 单次转换

// 温度传感器内存区域配置
#define TEMP_REG_REGION    0  // 寄存器区域索引
#define TEMP_REGION_COUNT  1  // 内存区域总数

// 温度传感器私有数据结构
typedef struct {
    device_mem_config_t mem_config;   // 内存配置
    pthread_mutex_t mutex;            // 互斥锁
    pthread_t update_thread;          // 温度更新线程
    int running;                      // 线程运行标志
} temp_sensor_data_t;

// 获取温度传感器操作接口
device_ops_t* get_temp_sensor_ops(void);

#endif // TEMP_SENSOR_H 