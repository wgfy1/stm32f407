/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/* USER CODE BEGIN 1 */
#include <stdint.h>

/**
 * MyRTC helper functions (HAL-based port of your STM32F1 code)
 * MyRTC_Time format: { year, month, day, hour, minute, second, weekday }
 */

void MyRTC_SetTimeFromArray(uint16_t MyRTC_Time[7])
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  sTime.Hours = (uint8_t)MyRTC_Time[3];
  sTime.Minutes = (uint8_t)MyRTC_Time[4];
  sTime.Seconds = (uint8_t)MyRTC_Time[5];
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;

  sDate.Date = (uint8_t)MyRTC_Time[2];
  sDate.Month = (uint8_t)MyRTC_Time[1];
  sDate.Year = (uint8_t)(MyRTC_Time[0] - 2000); /* HAL stores year as offset from 2000 */
  sDate.WeekDay = (uint8_t)(MyRTC_Time[6] ? MyRTC_Time[6] : RTC_WEEKDAY_MONDAY);

  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  /* mark RTC initialized in backup register */
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xA5A5U);
}

void MyRTC_ReadTimeToArray(uint16_t MyRTC_Time[7])
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); /* must be called after GetTime */

  MyRTC_Time[0] = 2000 + sDate.Year;
  MyRTC_Time[1] = sDate.Month;
  MyRTC_Time[2] = sDate.Date;
  MyRTC_Time[3] = sTime.Hours;
  MyRTC_Time[4] = sTime.Minutes;
  MyRTC_Time[5] = sTime.Seconds;
  MyRTC_Time[6] = sDate.WeekDay;
}


void MyRTC_Init(uint16_t MyRTC_Time[7])
{
  /* Allow access to backup domain (PWR) */
  HAL_PWR_EnableBkUpAccess();

  /* If not initialized, set time from provided array */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0xA5A5U)
  {
    MyRTC_SetTimeFromArray(MyRTC_Time);
  }
  else
  {
    /* already initialized, nothing to do */
  }
}

static uint8_t is_leap_year(uint16_t y) {
  return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

void MyRTC_SetFromEpoch(uint32_t epoch)
{
  /* Convert epoch (UTC seconds since 1970) to date/time components */
  uint32_t days = epoch / 86400;
  uint32_t rem = epoch % 86400;
  uint8_t hour = rem / 3600;
  uint8_t minute = (rem % 3600) / 60;
  uint8_t second = rem % 60;

  uint16_t year = 1970;
  while (1) {
    uint16_t diy = is_leap_year(year) ? 366 : 365;
    if (days >= diy) { days -= diy; year++; } else break;
  }
  static const uint8_t mdays_norm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  static const uint8_t mdays_leap[] = {31,29,31,30,31,30,31,31,30,31,30,31};
  const uint8_t *mdays = is_leap_year(year) ? mdays_leap : mdays_norm;
  uint8_t month = 1;
  while (days >= mdays[month-1]) { days -= mdays[month-1]; month++; }

  uint16_t arr[7];
  arr[0] = year;
  arr[1] = month;
  arr[2] = (uint16_t)(days + 1);
  arr[3] = hour;
  arr[4] = minute;
  arr[5] = second;
  /* weekday not calculated here, set 1 (Monday) as placeholder */
  arr[6] = 1;

  MyRTC_SetTimeFromArray(arr);
}

/* USER CODE END 1 */
