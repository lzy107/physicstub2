# 物理设备模拟器框架 (PhyMuTi)

这是一个用C语言实现的物理设备模拟器框架，支持动态加载设备插件、事件监控和动作触发等功能。框架包含完整的设备测试系统，可以对模拟设备进行自动化测试。

## 主要特性

- **插件系统**：支持动态加载设备插件
- **设备管理**：统一的设备注册和管理机制
- **地址空间**：独立的设备地址空间管理
- **事件监控**：支持地址变化监控和触发
- **动作系统**：可配置的动作规则和回调机制
- **线程安全**：所有操作都是线程安全的
- **测试框架**：自动化设备测试系统
- **规则引擎**：基于事件的规则触发系统

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
  - `flash/`: Flash设备实现
  - `fpga/`: FPGA设备实现
  - `temp_sensor/`: 温度传感器实现
  - `common/`: 公共组件

## 系统架构

### 核心组件

1. **设备管理器 (Device Manager)**
   - 管理设备类型和设备实例
   - 提供设备注册和查找接口
   - 维护设备类型和实例的生命周期

2. **动作管理器 (Action Manager)**
   - 管理动作规则
   - 处理规则触发和执行
   - 支持多种动作类型（写入、信号、回调）

3. **全局监视器 (Global Monitor)**
   - 监控设备地址变化
   - 触发相应的动作规则
   - 维护监视点列表

4. **设备内存 (Device Memory)**
   - 模拟设备内存和寄存器
   - 支持不同粒度的读写操作
   - 管理内存区域

5. **测试框架 (Device Test)**
   - 定义测试步骤和测试用例
   - 执行自动化测试
   - 验证测试结果

### 设备类型

框架目前实现了三种设备类型：

#### 1. Flash设备
- 支持读写操作和状态管理
- 寄存器：状态、控制、配置、地址和数据寄存器
- 支持写使能和数据存储功能

#### 2. FPGA设备
- 支持配置和控制操作
- 寄存器：状态、配置、控制和中断寄存器
- 支持内存映射和中断触发

#### 3. 温度传感器
- 支持温度读取和报警配置
- 寄存器：温度、配置、高温阈值和低温阈值寄存器
- 支持温度变化触发报警

## 使用示例

### 创建设备实例

```c
// 创建设备管理器
device_manager_t* dm = device_manager_init();

// 注册设备类型
device_type_register(dm, DEVICE_TYPE_FLASH, "FLASH", get_flash_device_ops());

// 创建设备实例
device_instance_t* flash = device_create(dm, DEVICE_TYPE_FLASH, 0);

// 使用设备
uint32_t value;
device_read(flash, FLASH_REG_STATUS, &value);
device_write(flash, FLASH_REG_CONTROL, FLASH_CTRL_WRITE);
```

### 设置监视点和规则

```c
// 创建全局监视器
global_monitor_t* gm = global_monitor_create(am, dm);

// 添加监视点
global_monitor_add_watch(gm, DEVICE_TYPE_FLASH, 0, FLASH_REG_STATUS);

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

### 运行测试

```c
// 定义测试步骤
static const test_step_t flash_test_steps[] = {
    {"设置写使能", FLASH_REG_CONTROL, FLASH_CTRL_WRITE, 0, 1, NULL, 1.0},
    {"读取状态", FLASH_REG_STATUS, 0, FLASH_STATUS_READY, 0, NULL, 1.0}
};

// 定义测试用例
static const test_case_t flash_test_case = {
    "Flash基本测试",
    DEVICE_TYPE_FLASH,
    0,
    flash_test_steps,
    sizeof(flash_test_steps) / sizeof(test_step_t),
    NULL,
    NULL
};

// 运行测试
run_test_case(dm, &flash_test_case);
```

## 扩展指南

### 添加新设备类型

1. 在`device_types.h`中添加设备类型ID
2. 创建设备头文件和实现文件
3. 实现设备操作接口（init, read, write, reset, destroy等）
4. 注册设备类型
5. 创建设备测试用例

### 添加新规则提供者

1. 创建规则表
2. 实现规则提供者接口
3. 注册规则提供者

## 关于编译系统

项目使用了一个自定义的Makefile构建系统，具有以下特点：

1. **临时目录构建** - 所有中间文件放在临时目录 `temp_build` 中，保持项目目录结构清晰
2. **扁平化头文件包含** - 编译时不再需要使用相对路径（如 `../../include/`）来包含头文件
3. **构建命令**：
   - `make` - 编译主程序
   - `make test` - 编译测试程序
   - `make clean` - 清理所有构建文件
   - `make run` - 运行主程序
   - `make run_test` - 运行测试程序
   - `make process_files` - 处理所有源代码文件，移除相对路径引用（永久修改源文件）

项目编译时会自动处理头文件包含路径，无需在源代码中使用复杂的相对路径。所有编译生成的中间文件都位于 `temp_build` 目录中，编译完成后可以使用 `make clean` 命令清理。


