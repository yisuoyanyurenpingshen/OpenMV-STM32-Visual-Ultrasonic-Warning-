#ifndef __HC_SR04_H
#define __HC_SR04_H

#include "stm32f1xx_hal.h"
#include "stdint.h"

/* 超声波引脚定义 */
#define HC_SR04_TRIG_PORT GPIOA
#define HC_SR04_TRIG_PIN  GPIO_PIN_8

#define HC_SR04_ECHO_PORT GPIOA
#define HC_SR04_ECHO_PIN  GPIO_PIN_9

/* 对外提供的变量 */
extern uint32_t echo_time_us;

/* 对外提供的函数 */
void HC_SR04_Init(void);
uint32_t HC_SR04_Read_cm(void);

#endif