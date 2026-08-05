# DJI Motor Driver

基于 STM32F407 的 DJI M3508 / M2006 / GM6020 电机 CAN 驱动与控制示例。

当前工程由 STM32CubeMX 生成基础外设代码，使用 CMake 构建；电机部分按“设备、管理器、控制器、CAN 底层”分层，支持每台电机独立选择电流、速度或位置模式。每条 CAN 总线由独立的 `DJI_MotorManager_t` 管理。

## 硬件与通信

- MCU：STM32F407IGH6
- CAN1、CAN2：经典 CAN，均为 1 Mbps
- CAN1：C620（M3508）或 C610（M2006）
- CAN2：GM6020，使用 CAN 电流控制模式

| 电机 | 反馈 ID | 控制帧 | 槽位 |
|---|---|---|---|
| M3508 / M2006 ID 1～4 | `0x201`～`0x204` | `0x200` | `ID - 1` |
| M3508 / M2006 ID 5～8 | `0x205`～`0x208` | `0x1FF` | `ID - 5` |
| GM6020 电流模式 ID 1～4 | `0x205`～`0x208` | `0x1FE` | `ID - 1` |
| GM6020 电流模式 ID 5～7 | `0x209`～`0x20B` | `0x2FE` | `ID - 5` |

接收过滤器使用掩码模式，接收 `0x200`～`0x20F` 标准数据帧。注册时会检查电机 ID、控制帧 ID、槽位以及重复占用。

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

- CAN1：4 个 M3508，ID 1～4，`0x200` 的四个槽位全部由本工程控制
- CAN2：2 个 GM6020，ID 1～2，使用 `0x1FE` 电流控制帧的槽位 0、1
- M3508：`100 rpm`，速度 PID 为 `Kp=3.0`、`Ki=0.02`，电流限幅 `±4000`
- GM6020：`50 rpm`，速度 PID 为 `Kp=20.0`、`Ki=0.02`，电流限幅 `±1000`
- 主循环每 `1 ms` 更新六个电机的控制器，并分别发送 CAN1/CAN2 分组帧

GM6020 需要使用支持电流环的固件，并在 RoboMaster Assistant 中开启电流环；否则不能使用 `0x1FE` / `0x2FE` 电流控制帧。

烧录后必须复位或重新上电，确认收到反馈后再让电机进入测试状态。

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

构建通过只能证明源码可编译和链接；CAN 收发、反馈解析、电机方向、限流与机械安全仍需在实际硬件上逐项验证。GM6020 和 M3508 测试总线必须分别接在 CAN2 与 CAN1。
