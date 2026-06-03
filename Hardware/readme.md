这里放硬件接线图
引脚连接
OpenMV 与 STM32 串口连接
OpenMV	STM32F103C8T6	说明
P4 / TX	PA10 / USART1_RX	OpenMV 向 STM32 发送命令
GND	GND	共地
本项目第一版只需要 OpenMV 向 STM32 单向发送数据，因此只接 P4 → PA10 和 GND → GND 即可。

HC-SR04 超声波模块
HC-SR04	STM32	说明
VCC	5V	模块供电
GND	GND	共地
TRIG	PB4	STM32 输出触发信号
ECHO	PB5	STM32 读取回声信号
注意：HC-SR04 的 ECHO 可能输出 5V 电平，STM32F103 是 3.3V 芯片，长期使用建议给 ECHO 加分压保护。

OLED 显示屏
OLED	STM32
SCL	PB6
SDA	PB7
VCC	3.3V / 5V
GND	GND
LED 分级报警
LED	STM32
LED1	PA4
LED2	PA5
LED3	PA6
