#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "main.h"
#include <stdint.h>
#include "stm32f4xx_hal.h"

// 电阻触摸屏控制引脚定义 - 使用CubeMX生成的标签
#define T_PEN    HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin)
#define T_CS(x)  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, (GPIO_PinState)(x))
#define T_CLK(x) HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, (GPIO_PinState)(x))
#define T_MOSI(x) HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, (GPIO_PinState)(x))
#define T_MISO   HAL_GPIO_ReadPin(T_MISO_GPIO_Port, T_MISO_Pin)

// 触摸屏状态定义
#define TP_PRES_DOWN 0x80
#define TP_CATH_PRES 0x40

// 触摸屏控制器结构体
typedef struct
{
    uint8_t (*init)(void);
    uint8_t (*scan)(uint8_t);
    void (*adjust)(void);
    uint16_t x[5];
    uint16_t y[5];
    uint8_t sta;
    float xfac;
    float yfac;
    int16_t xoff;
    int16_t yoff;
    uint8_t touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev;

// 函数声明
void TP_Write_Byte(uint8_t num);
uint16_t TP_Read_AD(uint8_t cmd);
static uint16_t TP_Read_XOY(uint8_t cmd);
static uint8_t TP_Read_XY(uint16_t *x, uint16_t *y);
static uint8_t TP_Read_XY2(uint16_t *x, uint16_t *y);
void TP_Drow_Touch_Point(uint16_t x, uint16_t y, uint16_t color);
void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color);
void TP_Save_Adjdata(void);
uint8_t TP_Get_Adjdata(void);
void TP_Adjust(void);
void TP_Adj_Info_Show(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, 
                     uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t fac);
uint8_t TP_Scan(uint8_t tp);
uint8_t TP_Init(void);

#endif