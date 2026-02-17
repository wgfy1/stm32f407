/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : timer_callbacks.c
  * @brief          : 定时器回调函数实现
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "timer_callbacks.h"
#include "gui_guider.h"
#include "rtc.h"
#include "touch.h"
#include "esp8266.h"
#include "stdio.h"

/* Private variables ---------------------------------------------------------*/

// 全局变量声明
extern lv_ui guider_ui;
extern uint16_t current_time[7];// 存储RTC时间的数组：年、月、日、时、分、秒、星期
extern char cached_time_str[10];// 用于缓存当前显示的时间字符串，避免重复更新
extern char cached_date_str[30];// 用于缓存当前显示的日期字符串，避免重复更新
extern _m_tp_dev tp_dev;

// 天气定时器计数
static uint32_t weather_timer_counter = 0;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  获取空闲内存
  * @retval 空闲内存大小（字节）
  */
uint32_t get_free_memory(void)
{
    extern char _end asm("_end");
    extern char _estack asm("_estack");
    
    char *heap_end = &_end;
    char *stack_ptr = (char*)__get_MSP();
    return (uint32_t)(stack_ptr - heap_end);
}

/**
  * @brief  时间更新定时器回调（每秒更新一次）
  * @param  timer: 定时器对象
  * @retval None
  */
void time_update_timer_cb(lv_timer_t * timer)
{
    if (lv_scr_act() != guider_ui.screen)
    {
        return;
    }
    
    MyRTC_ReadTimeToArray(current_time);
    uint8_t beijing_hour = current_time[3] + 8;
    if (beijing_hour >= 24)
    {
        beijing_hour -= 24;
    }
    
    extern char week_day[10];
    char new_time_str[10], new_date_str[40];
    sprintf(new_time_str, "%02d:%02d", beijing_hour, current_time[4]);
    sprintf(new_date_str, "%04d/%d/%d,%s", current_time[0], current_time[1], current_time[2], week_day);
    
    if (strcmp(new_time_str, cached_time_str) != 0 || strcmp(new_date_str, cached_date_str) != 0)
    {
        strcpy(cached_time_str, new_time_str);
        strcpy(cached_date_str, new_date_str);
        lv_label_set_text(guider_ui.screen_label_time, cached_time_str);
        lv_label_set_text(guider_ui.screen_label_date, cached_date_str);
    }
}

/**
  * @brief  天气更新定时器回调（每10分钟更新一次）
  * @param  timer: 定时器对象
  * @retval None
  */
void weather_update_timer_cb(lv_timer_t * timer)
{
    weather_timer_counter++;// 每100ms调用一次，所以计数器每10分钟达到6000
    
    // 每1分钟输出一次调试信息（600 * 100ms = 60秒）
    if (weather_timer_counter % 600 == 0)
    {
        printf("[定时器] 已过 %d 分钟，计数=%d，空闲内存=%u\r\n", 
               weather_timer_counter/600, weather_timer_counter, get_free_memory());
    }
    // 2分钟 = 120秒，定时器周期100ms，所以计数到1200
    if (weather_timer_counter >= 1200)
    {
        weather_timer_counter = 0;
        printf("[定时器] 10分钟到达，通知天气任务更新...\r\n");
        // 释放信号量，通知天气任务
        osSemaphoreRelease(weatherSemaphoreHandle);
    }
}

