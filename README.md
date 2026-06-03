# OpenMV-STM32-视觉触发式超声波分级报警系统
本项目基于 OpenMV 与 STM32F103C8T6 实现了一个视觉触发式超声波分级报警系统


系统由 OpenMV 摄像头识别画面中的红色目标。当检测到红色目标时，OpenMV 通过 UART 串口向 STM32 发送命令 `R`；当没有检测到目标时，发送命令 `N`。STM32 接收到命令后，根据目标状态决定是否启动 HC-SR04 超声波测距模块，并通过 OLED 显示当前状态、测距结果和报警等级，同时使用三颗 LED 实现分级距离报警。

本项目的核心思想是：

> OpenMV 负责“看”，STM32 负责“判断与执行”。

相比普通的超声波测距实验，本项目加入了视觉触发逻辑：只有当视觉模块识别到指定目标时，系统才启动测距和报警流程，更接近真实机器人或智能感知系统中的“视觉发现目标 + 距离确认 + 执行动作”结构。

---

## 功能效果

系统运行逻辑如下：

```text
OpenMV 识别红色目标
        ↓
看到红色 → 发送 R
没看到红色 → 发送 N
        ↓
STM32 接收串口命令
        ↓
收到 R → 启动 HC-SR04 测距
收到 N → 不测距，LED 全灭
        ↓
OLED 显示状态
        ↓
根据距离点亮 LED
```

LED 分级规则：

```text
距离 < 50 cm：亮 1 颗 LED
距离 < 30 cm：亮 2 颗 LED
距离 < 15 cm：亮 3 颗 LED
未识别到红色目标：LED 全灭，不进行测距
```

OLED 显示示例：

```text
Target: RED
Dist: 23 cm
Level: 2
CMD:R C:128
```

未识别到目标时：

```text
Target: NONE
Dist: --
Level: 0
CMD:N C:140
```

---

## 硬件清单

| 模块               | 作用             |
| ---------------- | -------------- |
| STM32F103C8T6    | 主控，下位机         |
| OpenMV H7 / H750 | 视觉识别，上位机       |
| HC-SR04          | 超声波测距          |
| OLED 显示屏         | 显示目标状态、距离和报警等级 |
| LED × 3          | 分级报警显示         |
| 杜邦线 / 面包板        | 电路连接           |
| 5V 电源            | 给模块供电          |

---

## 引脚连接

### OpenMV 与 STM32 串口连接

| OpenMV  | STM32F103C8T6    | 说明                  |
| ------- | ---------------- | ------------------- |
| P4 / TX | PA10 / USART1_RX | OpenMV 向 STM32 发送命令 |
| GND     | GND              | 共地                  |

本项目第一版只需要 OpenMV 向 STM32 单向发送数据，因此只接 `P4 → PA10` 和 `GND → GND` 即可。

---

### HC-SR04 超声波模块

| HC-SR04 | STM32 | 说明           |
| ------- | ----- | ------------ |
| VCC     | 5V    | 模块供电         |
| GND     | GND   | 共地           |
| TRIG    | PB4   | STM32 输出触发信号 |
| ECHO    | PB5   | STM32 读取回声信号 |

注意：HC-SR04 的 ECHO 可能输出 5V 电平，STM32F103 是 3.3V 芯片，长期使用建议给 ECHO 加分压保护。

---

### OLED 显示屏

| OLED | STM32     |
| ---- | --------- |
| SCL  | PB6       |
| SDA  | PB7       |
| VCC  | 3.3V / 5V |
| GND  | GND       |

---

### LED 分级报警

| LED  | STM32 |
| ---- | ----- |
| LED1 | PA4   |
| LED2 | PA5   |
| LED3 | PA6   |

---

## 通信协议

本项目使用极简串口协议：

| OpenMV 发送 | 含义           | STM32 动作   |
| --------- | ------------ | ---------- |
| `R`       | Red，检测到红色目标  | 启动超声波测距    |
| `N`       | None，没有检测到目标 | 不测距，LED 全灭 |

协议设计说明：

第一版项目只需要判断“是否检测到红色目标”，因此没有使用复杂的字符串协议，例如：

```text
R,1,err_x,area
```

而是直接使用单字符命令：

```text
R / N
```

这样可以避免字符串拼接、换行判断、`sscanf` 解析失败等问题，提高系统稳定性。

---

## OpenMV 端核心逻辑

OpenMV 负责识别红色目标，并通过 UART3 发送命令。

```python
import sensor
import time
from pyb import UART

uart = UART(3, 115200)

red_threshold = (30, 100, 20, 127, 0, 127)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
sensor.skip_frames(time=2000)

while True:
    img = sensor.snapshot()

    blobs = img.find_blobs([red_threshold],
                           pixels_threshold=300,
                           area_threshold=300,
                           merge=True)

    if blobs:
        max_blob = max(blobs, key=lambda b: b.pixels())
        img.draw_rectangle(max_blob.rect())
        img.draw_cross(max_blob.cx(), max_blob.cy())

        uart.write("R\n")
        print("RED")
    else:
        uart.write("N\n")
        print("NONE")

    time.sleep_ms(300)
```

---

## STM32 端核心逻辑

STM32 通过 USART1 接收 OpenMV 发来的 `R` / `N` 命令。

```c
void OpenMV_Receive_Task(void)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        if (HAL_UART_Receive(&huart1, &rx_ch, 1, 1) == HAL_OK)
        {
            uart_char_count++;
            last_cmd = rx_ch;

            if (rx_ch == 'R')
            {
                target_red = 1;
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            }
            else if (rx_ch == 'N')
            {
                target_red = 0;
            }
            else
            {
                // 忽略 \r 和 \n
            }
        }
        else
        {
            break;
        }
    }
}
```

主循环逻辑：

```c
while (1)
{
    OpenMV_Receive_Task();

    if (target_red == 1)
    {
        distance_cm = HC_SR04_Read_cm();
        led_level = Get_LED_Level(distance_cm);
    }
    else
    {
        distance_cm = 999;
        led_level = 0;
    }

    LED_Show_Level(led_level);
    OLED_Show_Status();

    HAL_Delay(100);
}
```

---

## 项目亮点

1. **视觉触发测距**
   不是让超声波一直测距，而是由 OpenMV 先识别目标，识别成功后再启动测距。

2. **上下位机分工清晰**
   OpenMV 负责视觉识别，STM32 负责接收命令、测距、显示和报警。

3. **通信协议简单稳定**
   使用 `R / N` 单字符协议，避免了复杂字符串解析带来的不稳定问题。

4. **OLED 实时显示系统状态**
   OLED 不仅用于显示结果，也用于调试串口命令和系统状态。

5. **模块化封装**
   超声波测距、LED 显示、OLED 显示等功能可以分别封装，便于后续扩展。

---

## 项目版本记录

### V0.1：OpenMV 串口发送测试

OpenMV 通过 P4/TX 发送 `hello stm32`，使用 USB-TTL 在电脑端验证串口数据。

### V0.2：STM32 串口接收测试

STM32 使用 USART1 接收 OpenMV 数据，收到字符后翻转 PC13 LED，验证通信链路。

### V0.3：尝试完整数据协议

尝试让 OpenMV 发送：

```text
R,1,err_x,area
```

STM32 使用 `rx_buf` 和 `sscanf` 解析视觉数据。

### V1.0：最终稳定版

将通信协议简化为：

```text
R = 识别到红色目标
N = 未识别到目标
```

STM32 收到 `R` 后启动 HC-SR04 测距，并通过 OLED 和 LED 显示报警状态。

---

## 后续升级方向

### 1. 加入完整视觉数据协议

后续可以重新加入：

```text
R,1,err_x,area
```

让 STM32 不仅知道是否有目标，还能知道目标在画面中的左右偏差和面积大小。

### 2. 加入舵机追踪

根据 `err_x` 控制 MG90S 舵机左右转动，实现视觉目标追踪。

### 3. 加入 ADC 阈值调节

使用电位器接入 STM32 ADC，通过旋钮调节报警距离阈值。

### 4. 接入智能小车

将本系统作为智能小车的视觉触发避障模块，实现目标识别、距离检测和运动控制。

---

## 项目总结

本项目完成了 OpenMV 与 STM32 之间的基础通信和多模块联动。系统通过 OpenMV 识别红色目标，并用极简 UART 协议通知 STM32；STM32 根据视觉命令触发超声波测距，并通过 OLED 和 LED 显示状态。

该项目适合作为 STM32、OpenMV、UART 通信、传感器融合和嵌入式系统设计的入门综合项目。
