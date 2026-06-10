# 智能焊台系统 V2 - V1.0 发布说明

本项目是基于开源 T12 焊台温控方案制作的智能焊台系统。V1.0 版本包含两部分：

- 控温板：ATmega328P 主控，负责 T12 烙铁头温度采样、PID 控温、OLED 显示、旋钮交互和焊台状态串口上报。
- 拓展板：STM32F103C8T6 主控，负责读取控温板状态、驱动三路继电器、TFT 彩屏显示、ESP-01S WebSocket 桥接和网页上位机控制。

V1.0 的目标是形成可烧录、可联调、可复刻的发布版本，删除前期诊断固件、临时测试工程和重复文档。

## 目录结构

```text
.
├─ 控温板
│  ├─ Hardware
│  │  ├─ SCH_智能焊台控制板_2026-06-07.pdf
│  │  ├─ BOM_T12焊台温控_智能焊台控制板_2026-06-07.xlsx
│  │  └─ Gerber_pcb_copy_2026-06-07.zip
│  └─ Firmware
│     └─ 1.7_uart_nonblocking
│        ├─ 1.7_uart_nonblocking.ino
│        ├─ Arduboy2.cpp / Arduboy2.h / ...
│        └─ build
│           └─ 1.7_uart_nonblocking.ino.hex
└─ 拓展板
   ├─ Hardware
   │  ├─ 原理图-焊台拓展板.pdf
   │  ├─ BOM_Board1_Schematic1_2026-06-07.xlsx
   │  └─ Gerber_PCB1_2026-06-07.zip
   ├─ Firmware
   │  └─ SmartSolder_Extension
   │     └─ C8T6
   │        └─ USER
   │           ├─ SmartSolder_Extension.uvprojx
   │           └─ Objects/SmartSolder_Extension.hex
   └─ Software
      ├─ SmartSolder_Host.html
      └─ ESP01S_WebSocket_Bridge
         └─ ESP01S_WebSocket_Bridge.ino
```

## 控温板固件

正式固件目录：

```text
控温板/Firmware/1.7_uart_nonblocking
```

正式烧录文件：

```text
控温板/Firmware/1.7_uart_nonblocking/build/1.7_uart_nonblocking.ino.hex
```

控温板固件基于原 `1.7` 开源焊台程序修改，保留原控温、显示和交互逻辑，并新增硬件 UART 状态上报。串口发送采用非阻塞状态机，每秒上报一帧设备状态。

### 控温板硬件接口

| 功能 | ATmega328P 引脚 | Arduino 引脚 | 说明 |
|---|---|---|---|
| 温度采样 | ADC0 | A0 | 烙铁头温度信号 |
| 输入电压采样 | ADC1 | A1 | 输入电压检测 |
| 蜂鸣器 | PD5 | D5 | 蜂鸣器输出 |
| 旋钮按键 | PD6 | D6 | 编码器按下 |
| 旋钮 A 相 | PD7 | D7 | 编码器输入 |
| 旋钮 B 相 | PB0 | D8 | 编码器输入 |
| 加热控制 | PB1 | D9 | MOSFET PWM 控制 |
| 手柄震动开关 | PB2 | D10 | 睡眠/唤醒检测 |
| 状态串口 TX | PD1/TXD | D1 | 向拓展板/上位机输出状态 |

ATmega328P-AU TQFP-32 封装中，`PD1/TXD` 对应物理引脚 31。

### 控温板串口参数

| 参数 | 配置 |
|---|---|
| 波特率 | 9600 bps |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验 | None |
| 流控 | None |
| 方向 | 控温板单向发送 |
| 帧结束 | LF，字节值 `0x0A` |
| 上报周期 | 约 1000 ms |

### 状态报文协议

报文格式：

```text
$T,<current_temp>,<set_temp>,<power_percent>,<mode>\n
```

示例：

```text
$T,326,350,42,H\n
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---|---|
| `$T` | 固定字符串 | 状态帧头 |
| `current_temp` | 十进制整数 | 当前温度，单位摄氏度；异常时为 `999` |
| `set_temp` | 十进制整数 | 当前设定温度，单位摄氏度 |
| `power_percent` | 十进制整数 | 加热功率百分比，范围 `0-100` |
| `mode` | 单个 ASCII 字符 | 运行状态 |
| `\n` | `0x0A` | 帧结束符 |

状态码：

| 状态码 | 名称 | 含义 |
|---|---|---|
| `E` | Error | 未插入手柄、烙铁头异常或温度检测异常 |
| `O` | Off | 关闭/停机模式 |
| `S` | Sleep | 睡眠模式 |
| `B` | Boost | Boost 加热模式 |
| `W` | Working | 稳定工作状态 |
| `H` | Heating | 正在加热 |
| `L` | Holding | 低功率维持或保温 |

正常工作示例：

```text
$T,320,320,12,W
$T,322,380,100,B
$T,381,320,0,L
```

未插入手柄示例：

```text
$T,999,320,0,E
```

### 串口稳定性修复

V1.0 固件解决了 UART 尾字节乱码问题。问题原因是原程序在 ADC 降噪采样时会进入 `SLEEP_MODE_ADC`，而 UART 最后一个字节可能仍在移位寄存器中发送。V1.0 在进入 ADC sleep 前等待 `TXC0` 发送完成，避免尾部字节被截断。

该修复不改变协议、不改变波特率，也不把整帧发送改成阻塞式。只有在准备进入 ADC sleep 且确实存在未完成 UART 字节时，才等待最后一个字节发送完成。

### 控温板烧录

推荐使用 USBasp 通过 ISP 烧录。芯片为 ATmega328P，外部晶振 16 MHz。

烧录正式固件：

```powershell
& "C:\Users\Antury\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin\avrdude.exe" `
-C "C:\Users\Antury\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\etc\avrdude.conf" `
-c usbasp -p m328p -B 10 `
-U flash:w:"D:\Workspace\Soldering stationV2\控温板\Firmware\1.7_uart_nonblocking\build\1.7_uart_nonblocking.ino.hex":i
```

推荐熔丝位：

```text
lfuse = 0xFF
hfuse = 0xD9
efuse = 0xFF
```

`lfuse=0xFF` 对应外部晶振运行且 CKDIV8 关闭，适配 16 MHz 时钟。若设备曾经被错误配置为内部 1 MHz 或 8 MHz，串口波特率会明显异常。

## 拓展板固件

正式工程：

```text
拓展板/Firmware/SmartSolder_Extension/C8T6/USER/SmartSolder_Extension.uvprojx
```

当前工程输出固件：

```text
拓展板/Firmware/SmartSolder_Extension/C8T6/USER/Objects/SmartSolder_Extension.hex
```

说明：V1.0 已更新源码中的 `FW_VERSION_TEXT`，并已使用 Keil 命令行重新生成 `Objects/SmartSolder_Extension.hex`。最近一次构建结果为 `0 Error(s), 0 Warning(s)`。

目标 MCU：

```text
STM32F103C8T6
```

工程使用 STM32F10x 标准外设库，开发环境为 Keil uVision。

### 拓展板主要功能

- USART3 接收控温板状态报文。
- USART1 调试输出，DMA 发送，避免阻塞主循环。
- USART2 与 ESP-01S 通信。
- 三路继电器输出，支持网页手动控制。
- 人体红外输入与自动模式策略。
- 1.3 寸 ST7789 240x240 TFT 显示。
- ESP-01S WebSocket 桥接网页上位机。

### 拓展板关键参数

| 功能 | 配置 |
|---|---|
| 固件版本显示 | `V1.0` |
| 控温板串口 | USART3 RX，9600 8N1 |
| 调试串口 | USART1，115200 8N1 |
| ESP-01S 串口 | USART2，115200 8N1 |
| 下位机在线超时 | 3000 ms |
| ESP 在线超时 | 10000 ms |
| 继电器触发 | 高电平有效 |
| 人体检测 | 高电平有效 |

### 拓展板引脚摘要

| 模块 | 引脚 | 说明 |
|---|---|---|
| 控温板 UART RX | PB11 / USART3_RX | 接 ATmega328P `PD1/TXD` 分压后信号 |
| 控温板 UART TX | PB10 / USART3_TX | 当前预留 |
| 调试串口 TX/RX | PA9 / PA10 | USART1 |
| ESP-01S TX/RX | PA2 / PA3 | USART2 |
| TFT SCL | PA5 / SPI1_SCK | ST7789 SPI 时钟 |
| TFT SDA | PA7 / SPI1_MOSI | ST7789 SPI 数据 |
| TFT BLK | PA8 / TIM1_CH1 | 背光 PWM |
| TFT RES | PB0 | 屏幕复位 |
| TFT DC | PB1 | 数据/命令选择 |
| 继电器 1 | PB13 | 照明灯 |
| 继电器 2 | PB12 | 排风扇 |
| 继电器 3 | PB14 | 预留负载 |
| 人体检测 | PA1 | HC-SR501 或兼容 PIR 输入 |

### 控温板到拓展板接线

推荐电平转换：

```text
ATmega328P PD1/TXD -- R上 5.1k --+-- STM32 PB11/RX
                                  |
                                R下 10k
                                  |
                                 GND

ATmega GND ------------------------ STM32 GND
```

该分压将 ATmega 5 V UART 高电平转换到约 3.3 V。若沿用早期 `10k/20k` 分压也能得到约 3.33 V，但驱动能力更弱；V1.0 推荐 `5.1k/10k`。

PCB 走线建议：

- UART 走线远离 24 V 输入、继电器线圈、加热 MOSFET 和大电流回路。
- ATmega 与 STM32 必须共地。
- STM32 RX 旁可预留 100R 串联电阻位。
- STM32 RX 旁可预留 10k 上拉/下拉焊盘，默认不焊。

### STM32 接收策略

STM32 端不要假设每次中断或每次读取都能得到完整一帧。正确策略：

1. 接收字节流。
2. 遇到 `$` 重新同步帧头。
3. 只在帧内接收 ASCII 可打印字符。
4. 遇到 LF `0x0A` 结束一帧。
5. 最大帧长 32 字节，超长直接丢弃并等待下一个 `$`。
6. 解析时要求字段数量为 5。
7. 只把合法帧用于刷新设备在线状态。

在线判断：

```text
收到合法 $T 帧：在线
连续 3 秒未收到合法 $T 帧：离线
收到坏帧：丢弃，不立即判离线
```

### 拓展板网页和 ESP-01S

网页上位机：

```text
拓展板/Software/SmartSolder_Host.html
```

ESP-01S 桥接固件：

```text
拓展板/Software/ESP01S_WebSocket_Bridge/ESP01S_WebSocket_Bridge.ino
```

ESP-01S 提供 WebSocket 服务：

```text
ws://<ESP_IP>:81/
```

网页通过 ESP-01S 与 STM32 交换状态和控制命令。STM32 再负责继电器控制、状态汇总和屏幕显示。

## 硬件文件

### 控温板

| 文件 | 说明 |
|---|---|
| `控温板/Hardware/SCH_智能焊台控制板_2026-06-07.pdf` | 控温板原理图 |
| `控温板/Hardware/BOM_T12焊台温控_智能焊台控制板_2026-06-07.xlsx` | 控温板 BOM |
| `控温板/Hardware/Gerber_pcb_copy_2026-06-07.zip` | 控温板 Gerber |

### 拓展板

| 文件 | 说明 |
|---|---|
| `拓展板/Hardware/原理图-焊台拓展板.pdf` | 拓展板原理图 |
| `拓展板/Hardware/BOM_Board1_Schematic1_2026-06-07.xlsx` | 拓展板 BOM |
| `拓展板/Hardware/Gerber_PCB1_2026-06-07.zip` | 拓展板 Gerber |

## V1.0 验收项

控温板：

- 使用 USBasp 可烧录 `1.7_uart_nonblocking.ino.hex`。
- ATmega328P 工作于 16 MHz 外部晶振。
- 未插手柄时串口输出 `current_temp=999`、`mode=E`。
- 插入手柄后温度可稳定调节并输出 `$T` 状态帧。
- 串口尾字节不再出现乱码或截断。

拓展板：

- USART3 能稳定解析控温板 `$T` 帧。
- 超过 3 秒未收到合法帧时判定控温板离线。
- 三路继电器可通过网页或本地逻辑控制。
- TFT 显示温度、模式、在线状态、继电器状态和温度曲线。
- ESP-01S WebSocket 桥接可连接网页上位机。

## 已知限制

- 控温板串口目前只支持状态上报，不支持拓展板向控温板下发控制命令。
- 状态帧未包含校验和，短距离板间通信依靠帧头、字段数量、字段范围和超时机制保证可靠性。
- TFT 当前主要使用轻量 ASCII 字体；中文显示需要额外字库支持。
- 拓展板自动模式依赖人体检测输入，实际延时策略需结合现场使用习惯调整。

## 发布清理说明

V1.0 发布包已移除：

- 控温板串口诊断固件和短报文测试固件。
- 控温板临时编译探测目录。
- 控温板完整第三方依赖仓库副本。
- 拓展板早期 `BodyIR` 实验工程。
- 分散且重复的旧版 Markdown 说明文档。

保留内容仅包括 V1.0 复刻、烧录、联调和发布所需的硬件文件、正式固件、网页上位机和 ESP-01S 桥接固件。
