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
#include "stm32f4xx_hal_pwr.h"

volatile uint8_t weather_update_pending = 0;// 天气更新标志，供LVGLTask检查

// 背光控制变量
#define BACKLIGHT_TIMEOUT_MS    30000
volatile uint32_t lastActivityTime = 0;
volatile uint8_t backlightState = 1;

void Backlight_On(void) {
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
    backlightState = 1;
    lastActivityTime = HAL_GetTick();
}

void Backlight_Off(void) {
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);
    backlightState = 0;
}

void Backlight_Update(void) {
    uint32_t elapsed = HAL_GetTick() - lastActivityTime;
    if (elapsed > BACKLIGHT_TIMEOUT_MS && backlightState) {
        Backlight_Off();
        printf("[背光] 无操作超时，关闭背光省电\r\n");
    }
}

void Backlight_Activity(void) {
    lastActivityTime = HAL_GetTick();
    if (!backlightState) {
        Backlight_On();
        printf("[背光] 检测到活动，开启背光\r\n");
    }
}
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
// 任务句柄
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for lvglTask */
// 任务句柄
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


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartLVGLTask(void *argument);
void StartWeatherTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);// 空闲任务钩子函数

/* USER CODE BEGIN 2 */

void vApplicationIdleHook( void )
{
    /* 进入睡眠模式，等待中断唤醒 */
 HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  // 唤醒后继续执行任务
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

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

  /* Create the semaphores(s) */
  /* creation of weatherSemaphore */
  weatherSemaphoreHandle = osSemaphoreNew(1, 1, &weatherSemaphore_attributes);

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

  /* creation of lvglTask */
  lvglTaskHandle = osThreadNew(StartLVGLTask, NULL, &lvglTask_attributes);

  /* creation of weatherTask */
  weatherTaskHandle = osThreadNew(StartWeatherTask, NULL, &weatherTask_attributes);

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
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  Backlight_On();
  for(;;)
  {
    Backlight_Update();
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartLVGLTask */
/**
* @brief Function implementing the lvglTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLVGLTask */
void StartLVGLTask(void *argument)
{
  /* USER CODE BEGIN StartLVGLTask */
    extern void update_weather_display(void);// 声明更新天气显示的函数
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
    }    osDelay(1);
  }
  /* USER CODE END StartLVGLTask */
}

/* USER CODE BEGIN Header_StartWeatherTask */
/**
* @brief Function implementing the weatherTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWeatherTask */
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
      
      // 通过MQTT发布天气数据
      extern int temperature;
      extern int humidity;
      char mqtt_data[128];
      snprintf(mqtt_data, sizeof(mqtt_data), 
               "{\"temp\":%d,\"humidity\":%d,\"location\":\"%s\"}",
               temperature, humidity, YOUR_LOCATION);
      ESP8266_MQTT_Publish(mqtt_data);
      
      osDelay(10);
      }
      }
  /* USER CODE END StartWeatherTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

