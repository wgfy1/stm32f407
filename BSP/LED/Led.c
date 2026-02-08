
#include "Led.h"
#include "main.h"

void Led_Init(void)
{
    
}

void Led2_On(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

void Led2_Off(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}

void Led3_On(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
}

void Led3_Off(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
}

//翻转LED2
void led2_toggle(void)
{
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}
