/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for lvglTask */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
  .name = "lvglTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Definitions for weatherTask */
osThreadId_t weatherTaskHandle;
const osThreadAttr_t weatherTask_attributes = {
  .name = "weatherTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for weatherSemaphore */
osSemaphoreId_t weatherSemaphoreHandle;
const osSemaphoreAttr_t weatherSemaphore_attributes = {
  .name = "weatherSemaphore"
};

/* 天气更新标志 - 由 weatherTask 设置，LVGLTask 执行 */
volatile uint8_t weather_update_pending = 0;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartLVGLTask(void *argument);
void StartWeatherTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  weatherSemaphoreHandle = osSemaphoreNew(1, 0, &weatherSemaphore_attributes);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  lvglTaskHandle = osThreadNew(StartLVGLTask, NULL, &lvglTask_attributes);
  weatherTaskHandle = osThreadNew(StartWeatherTask, NULL, &weatherTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
// 默认任务，用于处理其他任务的基本操作
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/* USER CODE BEGIN Header_StartLVGLTask */
/**
  * @brief  Function implementing the lvglTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLVGLTask */
// LVGL任务，用于处理LVGL事件
void StartLVGLTask(void *argument)
{
  /* USER CODE BEGIN StartLVGLTask */
  extern void update_weather_display(void);
  extern void update_main_screen_date(void);
  
  /* Infinite loop */
  for(;;)
  {
    lv_timer_handler();
    
    // 检查是否有天气更新请求（在lv_timer_handler之后执行）
    if (weather_update_pending) {
      weather_update_pending = 0;
      printf("[LVGLTask] 开始更新天气显示...\r\n");
      update_weather_display();
      update_main_screen_date();
      printf("[LVGLTask] 天气显示更新完成\r\n");
    }
    
    osDelay(1);
  }
  /* USER CODE END StartLVGLTask */
}

/* USER CODE BEGIN Header_StartWeatherTask */
/**
  * @brief  Function implementing the weatherTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartWeatherTask */
// 天气任务，用于获取天气数据
void StartWeatherTask(void *argument)
{
  /* USER CODE BEGIN StartWeatherTask */
  /* Infinite loop */
  for(;;)
  {
    // 等待信号量
    if(osSemaphoreAcquire(weatherSemaphoreHandle, 600000) == osOK) {
      printf("[天气任务] 开始获取天气数据...\r\n");
      ESP8266_GetWeather();     // 获取天气数据
      printf("[天气任务] 数据获取完成，请求UI更新\r\n");
      weather_update_pending = 1;  // 设置标志，让LVGLTask更新UI
      osDelay(10);  // 让出CPU，让LVGLTask有机会执行
    }
  }
  /* USER CODE END StartWeatherTask */
}

