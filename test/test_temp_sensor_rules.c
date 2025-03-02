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

int main() {
    // create temp sensor device
    device_instance_t* temp_sensor = device_create(g_device_manager, DEVICE_TYPE_TEMP_SENSOR, 0);
    if (!temp_sensor) {
        printf("错误: 无法创建温度传感器设备\n");
        return 1;
    }
    //  addr 0x4  write 3
    device_write(temp_sensor, 0x4, 3);
    // check addr 0x8  是否等于 5
    uint32_t value = 0;
    device_read(temp_sensor, 0x8, &value);
    printf("addr 0x8  read %d\n", value);
    if (value != 5) {
        printf("错误: addr 0x8  read %d 不等于 5\n", value);
        return 1;
    }else{
        printf("正确: addr 0x8  read %d 等于 5\n", value);
    }
    return 0;
} 