#include "hc_sr04.h"

/* Echo 高电平持续时间，单位 us */
uint32_t echo_time_us = 0;

/* 初始化 DWT 微秒计时 */
void HC_SR04_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* 微秒延时函数，只在 hc_sr04.c 内部使用 */
static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);

    while ((DWT->CYCCNT - start) < ticks);
}

/* 读取距离，单位 cm；返回 999 表示测距失败 */
uint32_t HC_SR04_Read_cm(void)
{
    uint32_t timeout = 0;
    uint32_t start_time = 0;
    uint32_t end_time = 0;
    uint32_t duration = 0;
    uint32_t distance = 0;

    /* Trig 先拉低，稳定一下 */
    HAL_GPIO_WritePin(HC_SR04_TRIG_PORT, HC_SR04_TRIG_PIN, GPIO_PIN_RESET);
    delay_us(2);

    /* Trig 拉高 10us，触发超声波 */
    HAL_GPIO_WritePin(HC_SR04_TRIG_PORT, HC_SR04_TRIG_PIN, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(HC_SR04_TRIG_PORT, HC_SR04_TRIG_PIN, GPIO_PIN_RESET);

    /* 等 Echo 从低变高 */
    timeout = 30000;
    while (HAL_GPIO_ReadPin(HC_SR04_ECHO_PORT, HC_SR04_ECHO_PIN) == GPIO_PIN_RESET)
    {
        if (timeout-- == 0)
        {
            echo_time_us = 0;
            return 999;
        }

        delay_us(1);
    }

    start_time = DWT->CYCCNT;

    /* 等 Echo 从高变低 */
    timeout = 30000;
    while (HAL_GPIO_ReadPin(HC_SR04_ECHO_PORT, HC_SR04_ECHO_PIN) == GPIO_PIN_SET)
    {
        if (timeout-- == 0)
        {
            echo_time_us = 0;
            return 999;
        }

        delay_us(1);
    }

    end_time = DWT->CYCCNT;

    /* CPU 周期数换算成 us */
    duration = (end_time - start_time) / (HAL_RCC_GetHCLKFreq() / 1000000);

    echo_time_us = duration;

    /* 距离 cm ≈ Echo 高电平时间 us / 58 */
    distance = duration / 58;

    return distance;
}