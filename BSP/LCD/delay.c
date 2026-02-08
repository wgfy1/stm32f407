#include "delay.h"

/**
 * @brief  微秒级延时
 * @param  us: 要延时的微秒数
 * @retval 无
 */
void delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;            /* LOAD的值 */
    ticks = us * (SystemCoreClock / 1000000);    /* 需要的节拍数 */
    told = SysTick->VAL;                         /* 刚进入时的计数器值 */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told) tcnt += told - tnow;
            else tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks) break;            /* 时间超过/等于要延迟的时间,则退出 */
        }
    }
}

/**
 * @brief  毫秒级延时
 * @param  ms: 要延时的毫秒数
 * @retval 无
 */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}