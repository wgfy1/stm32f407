# F407project

STM32F407 项目（基于 STM32CubeMX + HAL + LVGL）。

说明：
- 本项目使用 CMake 构建，适用于 `arm-none-eabi` 工具链。

主要特性：
- 基于 STM32F407 的 HAL 外设初始化和示例代码
- LVGL 图形库集成，包含显示与触摸驱动移植
- ESP8266 串口驱动示例、DHT11 传感器和 LCD 驱动


项目结构概览：
- `Core/` — 主程序源码与 HAL 初始化
- `Drivers/` — CMSIS 与 HAL 驱动
- `BSP/` — 板级支持包（LCD、ESP8266、DHT11 等）
- `LVGL/` — LVGL 库与示例
- `build/` — 构建输出目录（本地生成）

