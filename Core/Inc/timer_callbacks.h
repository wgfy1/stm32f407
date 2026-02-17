/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : timer_callbacks.h
  * @brief          : 定时器回调函数声明
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

#ifndef TIMER_CALLBACKS_H
#define TIMER_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "cmsis_os.h"

// 外部声明FreeRTOS对象
extern osSemaphoreId_t weatherSemaphoreHandle;

// 回调函数声明
void time_update_timer_cb(lv_timer_t * timer);// 时间更新定时器回调（每秒更新一次）
void weather_update_timer_cb(lv_timer_t * timer);// 天气更新定时器回调（每100ms检查一次，10分钟更新一次）
// 获取空闲内存
uint32_t get_free_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_CALLBACKS_H */
