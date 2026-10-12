# DJI Motor Driver

基于 STM32F407 的 DJI M3508 / M2006 / GM6020 电机 CAN 驱动与控制示例。

当前工程由 STM32CubeMX 生成基础外设代码，使用 CMake 构建；电机部分按“设备、管理器、控制器、CAN 底层”分层，支持每台电机独立选择电流、速度或位置模式。每条 CAN 总线由独立的 `DJI_MotorManager_t` 管理。

## 硬件与通信

- MCU：STM32F407IGH6
- CAN1、CAN2：经典 CAN，均为 1 Mbps
- 每条 CAN 总线均可混装 M3508、M2006、GM6020，接收按各电机的 `feedback_id` 匹配
- GM6020 使用 CAN 电流控制模式

| 电机 | 反馈 ID | 控制帧 | 槽位 |
|---|---|---|---|
| M3508 / M2006 ID 1～4 | `0x201`～`0x204` | `0x200` | `ID - 1` |
| M3508 / M2006 ID 5～8 | `0x205`～`0x208` | `0x1FF` | `ID - 5` |
| GM6020 电流模式 ID 1～4 | `0x205`～`0x208` | `0x1FE` | `ID - 1` |
| GM6020 电流模式 ID 5～7 | `0x209`～`0x20B` | `0x2FE` | `ID - 5` |

接收过滤器使用掩码模式，接收 `0x200`～`0x20F` 标准数据帧。型号注册函数自动生成反馈 ID：M3508/M2006 为 `0x200 + ID`，GM6020 为 `0x204 + ID`。同一 manager 内会检查反馈 ID 和控制帧槽位是否重复；不同型号可以复用电机 ID，但反馈 ID 不能冲突。

同一组控制帧必须只有一个控制任务发送。未注册槽位会发送零，因此该 manager 必须拥有该控制帧影响的全部实际电机。

## 目录

```text
Core/UserModules/
├── Algorithm/  # PID
├── BSP/        # CAN 收发和过滤器
├── Control/    # 电流、速度、位置控制
└── Devices/    # 电机对象、M3508/M2006/GM6020 和电机管理器
```

## 当前测试配置

`Core/Src/main.c` 当前进行低速速度闭环测试：

- CAN1：1 个 M3508，ID7，反馈 `0x207`，控制帧 `0x1FF` 槽位 2
- 目标速度：`-500 rpm`，速度 PID 为 `Kp=3.0`、`Ki=0.02`、`Kd=0`
- 电流命令限幅：`±6000`（协议数值，并非 mA）；`motor_direction=1`
- 主循环每 `1 ms` 更新该电机并发送 CAN1 分组帧；CAN2 当前未注册或启动电机管理器

`0x1FF` 其余槽位为零，测试时会影响同一 CAN1 上的 ID5、6、8。推荐混装配置仍可采用 CAN1 的 4 台 M3508（ID1～4）及 CAN2 的 2 台 GM6020（ID1～2）加 1～2 台 M2006（ID1～2），需要在主函数中注册并更新相应电机。

GM6020 需要使用支持电流环的固件，并在 RoboMaster Assistant 中开启电流环；否则不能使用 `0x1FE` / `0x2FE` 电流控制帧。

烧录后必须复位或重新上电，确认收到反馈后再让电机进入测试状态。

## 构建

电机方向在 `MotorControl_Init()` 中通过单个 `motor_direction` 参数设置，只接受 `1` 或 `-1`，注册时默认为 `1`。设置为 `-1` 会同时反转发送电流、速度反馈、电流反馈和累计编码器增量；原始 `encoder` 保留电调返回值。方向应在初始化时设置，运行中变更方向前应停止电机并重新初始化控制状态。

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

构建通过只能证明源码可编译和链接；CAN 收发、反馈解析、电机方向、限流与机械安全仍需在实际硬件上逐项验证。当前单电机测试接在 CAN1。
