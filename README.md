# DJI Motor Driver

基于 STM32F407 的 DJI M3508 / M2006 电机 CAN 驱动与控制示例。

当前工程由 STM32CubeMX 生成基础外设代码，使用 CMake 构建；电机部分按“设备、管理器、控制器、CAN 底层”分层，支持每台电机独立选择电流、速度或位置模式。

## 硬件与通信

- MCU：STM32F407IGH6
- CAN：CAN1，经典 CAN，1 Mbps
- 电调：C620（M3508）或 C610（M2006）
- M3508 反馈 ID：`0x201`～`0x208`
- 控制帧：`0x200` 控制 ID 1～4，`0x1FF` 控制 ID 5～8

同一组控制帧必须只有一个控制任务发送。测试单个 ID 1 电机时，确认同一 CAN 总线上的 ID 2～4 可以接受零电流命令。

## 目录

```text
Core/UserModules/
├── Algorithm/  # PID
├── BSP/        # CAN 收发和过滤器
├── Control/    # 电流、速度、位置控制
└── Devices/    # 电机对象、M3508/M2006 和电机管理器
```

## 当前测试配置

`Core/Src/main.c` 当前只注册 1 号 M3508：

- 控制模式：`MOTOR_MODE_SPEED`
- 目标速度：`1000 rpm`
- 速度 PID：`Kp=3.0`，`Ki=0.02`，`Kd=0`
- 电流限幅：`±4000`

烧录后必须复位或重新上电，确保 MCU 执行的是最新固件。

## 构建

需要安装 ARM GNU Toolchain、CMake 和 Ninja。

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

生成文件位于：

```text
build/Debug/3508.elf
```

## OpenOCD 烧录

工程内提供常用 ST-Link 配置。此工程为 F407，请选择：

```text
openocd/stm32_f/stlink_stm32f4.cfg
```

配置文件依赖 OpenOCD 自带的 `interface/stlink.cfg` 与 `target/stm32f4x.cfg`。烧录后执行 Reset，再进行电机测试。

## 验证边界

构建通过只能证明源码可编译和链接；CAN 收发、反馈解析、电机方向、限流与机械安全仍需在实际硬件上逐项验证。
