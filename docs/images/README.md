# 图片目录说明

本目录用于存放 README 中的复刻过程图片。

建议使用英文文件名，避免部分 Markdown 查看器或网页环境对中文路径兼容不好。

推荐文件名：

| 图片内容 | 文件名 |
|---|---|
| 整机效果图 | `device-overview.jpg` |
| 系统架构图 | `system-architecture.png` |
| 控温板正面 | `control-board-front.jpg` |
| 控温板背面 | `control-board-back.jpg` |
| 拓展板正面 | `extension-board-front.jpg` |
| 拓展板背面 | `extension-board-back.jpg` |
| 控温板 ISP 接线 | `atmega-isp-wiring.jpg` |
| STM32 SWD 接线 | `stm32-swd-wiring.jpg` |
| ESP-01S 烧录接线 | `esp8266-flash-wiring.jpg` |
| 电源测试点 | `power-test-points.jpg` |
| TFT 界面 | `tft-ui.jpg` |
| 网页界面 | `web-ui.png` |

在 README 中插入图片示例：

```md
![TFT 界面](docs/images/tft-ui.jpg)
```

## 当前已整理图片

原始照片保留在 `docs/images/焊台照片/`，压缩后的命名图片位于 `docs/images/`。

压缩参数：

- 长边最大 1600 px。
- JPEG quality 82。
- 去除 EXIF，启用 progressive JPEG。
- 原图不覆盖、不删除。

| 原始文件 | 压缩后文件 | 建议用途 |
|---|---|---|
| `MVIMG_20260610_173456.jpg` | `dual-fan-module.jpg` | 双风扇模块 |
| `MVIMG_20260610_173514.jpg` | `control-panel-front.jpg` | 控温面板正面 |
| `MVIMG_20260610_173520.jpg` | `control-board-front.jpg` | 控温板正面 |
| `MVIMG_20260610_173606.jpg` | `usbasp-programmer-front.jpg` | USBasp 下载器 |
| `MVIMG_20260610_173636.jpg` | `usbasp-programmer-connected.jpg` | USBasp 接线状态 |
| `MVIMG_20260610_173639.jpg` | `control-board-uart-wiring.jpg` | 控温板串口/接线细节 |
| `MVIMG_20260610_173703.jpg` | `extension-board-front.jpg` | 拓展板正面 |
| `MVIMG_20260610_173708.jpg` | `extension-board-back.jpg` | 拓展板背面 |
| `MVIMG_20260610_174438.jpg` | `power-switch-module.jpg` | 电源开关模块 |
| `MVIMG_20260610_174444.jpg` | `control-board-power-section.jpg` | 控温板电源区域 |
| `MVIMG_20260610_174517.jpg` | `power-cable.jpg` | 电源线 |
| `MVIMG_20260610_174550.jpg` | `t12-handle-assembled.jpg` | T12 手柄总成 |
| `MVIMG_20260610_174611.jpg` | `t12-tip-and-handle.jpg` | T12 烙铁头与手柄 |
| `MVIMG_20260610_174648.jpg` | `t12-handle-internal-1.jpg` | T12 手柄内部接线 1 |
| `MVIMG_20260610_174712.jpg` | `t12-handle-internal-2.jpg` | T12 手柄内部接线 2 |
| `MVIMG_20260610_174719.jpg` | `t12-handle-internal-3.jpg` | T12 手柄内部接线 3 |
| `MVIMG_20260610_174950.jpg` | `control-panel-assembled.jpg` | 控温面板装配 |
| `MVIMG_20260610_175025.jpg` | `solder-station-wiring-overview.jpg` | 焊台接线总览 |
| `MVIMG_20260610_181543.jpg` | `extension-board-test-setup.jpg` | 拓展板联调场景 |
| `MVIMG_20260610_182011.jpg` | `boards-overview.jpg` | 控温板与拓展板对比 |
| `MVIMG_20260610_182045.jpg` | `integration-test-setup.jpg` | 系统联调场景 |
| `MVIMG_20260610_184221.jpg` | `device-overview.jpg` | 整机/工作台总览 |
