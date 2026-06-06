# ESP01S WebSocket Bridge V0.3

该固件用于 ESP-01S，把网页 WebSocket 与 STM32 USART2 文本协议互相转发。

## 接线

```text
ESP TX  -> STM32 PA3 / USART2_RX
ESP RX  -> STM32 PA2 / USART2_TX
ESP GND -> STM32 GND
ESP VCC -> 3.3V 稳定电源
```

## Arduino IDE

开发板选择：

```text
Generic ESP8266 Module
Flash Mode: DOUT
Flash Size: 1MB
CPU Frequency: 80 MHz
Upload Speed: 115200
```

需要安装：

```text
esp8266 by ESP8266 Community
WebSockets by Markus Sattler
```

## WebSocket

离线网页连接：

```text
ws://<device-ip>:81/
```

## STM32 -> ESP

状态上报：

```text
STAT {"fw":"V0.3","mode":"MANUAL","espOnline":1,"webOnline":1,"lowerOnline":1,"temp":320,"setTemp":350,"vin":24,"relay":{"light":1,"fan":0,"aux":0},"ip":"192.168.1.88","debug":"T:320,S:350,V:24"}
```

命令应答：

```text
ACK seq=1 ok=1 reason=NONE
```

## ESP -> STM32

网络状态：

```text
IP=192.168.1.88
WS=1
DIAG WIFI_OK ip=192.168.1.88
```

网页控制命令：

```text
CMD seq=1 target=light value=1
CMD seq=2 target=fan value=0
CMD seq=3 target=aux value=1
CMD seq=4 target=mode value=0
CMD seq=5 target=all value=0
```

V0.3 中 `mode=0` 为手动模式，`mode=1` 为自动模式。自动模式的人体传感器联动入口已保留，但 STM32 默认未启用。
