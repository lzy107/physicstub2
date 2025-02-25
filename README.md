# Device Simulator Framework

这是一个用C语言实现的设备模拟器框架，支持动态加载设备插件、事件监控和动作触发等功能。框架包含完整的设备测试系统，可以对模拟设备进行自动化测试。

## 主要特性

- 插件系统：支持动态加载设备插件
- 设备管理：统一的设备注册和管理机制
- 地址空间：独立的设备地址空间管理
- 事件监控：支持地址变化监控和触发
- 动作系统：可配置的动作规则和回调机制
- 线程安全：所有操作都是线程安全的
- 测试框架：自动化设备测试系统
- 规则引擎：基于事件的规则触发系统

## 构建方法

```bash
# 清理构建目录
make clean

# 构建所有目标（包括主程序和插件）
make all

# 构建并运行程序
make clean && make all && ./build/program
```

## 目录结构

- `src/`: 核心源代码
  - `action_manager.c`: 动作管理器实现
  - `address_space.c`: 地址空间管理
  - `device_memory.c`: 设备内存管理
  - `device_registry.c`: 设备注册表
  - `device_test.c`: 设备测试框架
  - `device_types.c`: 设备类型管理
  - `global_monitor.c`: 全局监视器
  - `main.c`: 主程序入口
- `include/`: 头文件
  - `action_manager.h`: 动作管理器接口
  - `address_space.h`: 地址空间接口
  - `device_memory.h`: 设备内存接口
  - `device_registry.h`: 设备注册表接口
  - `device_test.h`: 设备测试接口
  - `device_types.h`: 设备类型接口
  - `global_monitor.h`: 全局监视器接口
- `plugins/`: 设备插件
  - `flash_device.c/h`: Flash设备实现
  - `fpga_device.c/h`: FPGA设备实现
  - `temp_sensor.c/h`: 温度传感器实现
  - `flash_device_test.c`: Flash设备测试用例
  - `fpga_device_test.c`: FPGA设备测试用例
  - `temp_sensor_test.c`: 温度传感器测试用例
  - `device_tests.c/h`: 设备测试集成
  - `test_rule_provider.c/h`: 测试规则提供者
- `build/`: 构建输出目录

## 设备类型

框架目前实现了三种设备类型：

### 1. Flash设备
- 支持读写操作和状态管理
- 寄存器：状态、控制、配置、地址和数据寄存器
- 支持写使能和数据存储功能

### 2. FPGA设备
- 支持配置和控制操作
- 寄存器：状态、配置、控制和中断寄存器
- 支持内存映射和中断触发

### 3. 温度传感器
- 支持温度读取和报警配置
- 寄存器：温度、配置、高温阈值和低温阈值寄存器
- 支持温度变化触发报警

## 测试框架

框架包含完整的设备测试系统，支持：

- 测试步骤定义：支持读写操作和值验证
- 测试用例管理：组织多个测试步骤
- 测试套件执行：批量运行多个测试用例
- 自动化测试：支持设置和清理函数

### 测试用例示例

每个设备都有对应的测试用例，例如Flash设备测试：

1. 设置写使能
2. 设置地址寄存器
3. 写入数据寄存器
4. 读取数据
5. 触发规则
6. 验证结果

## 规则系统

框架实现了基于事件的规则触发系统：

- 规则定义：触发条件和动作目标
- 规则提供者：模块化规则管理
- 动作类型：写入、信号和回调
- 监视点：监控地址变化

### 规则示例

```c
// 创建目标处理动作
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
global_monitor_setup_watch_rule(gm, DEVICE_TYPE_FPGA, 0, FPGA_IRQ_REG, 
                               0x01, 0xFF, target);
```

## 使用示例

```c
// 创建设备实例
device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);

// 设置监视点
global_monitor_add_watch(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_STATUS);

// 运行测试
run_test_case(dm, &flash_test_case);
```

## 扩展指南

要添加新的设备类型：

1. 创建设备头文件和实现文件
2. 实现设备操作接口（init, read, write, reset, destroy）
3. 注册设备类型
4. 创建设备测试用例
5. 添加到测试套件

## 许可证

MIT License 