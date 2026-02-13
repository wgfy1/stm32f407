/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include<stdio.h>
#include"delay.h"
#include"lvgl.h"
#include"lv_port_indev.h"
#include"lv_port_disp.h"
#include"lcd.h"
#include"esp8266.h"
#include"string.h"
#include "gui_guider.h"
#include"events_init.h"
#include "rtc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
lv_ui guider_ui;
extern long long time;
uint16_t current_time[7];
char cached_time_str[10] = "00:00";
char cached_date_str[30] = "2024/1/1";

static void time_update_timer_cb(lv_timer_t * timer)
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
    char new_time_str[10], new_date_str[30];
    sprintf(new_time_str, "%02d:%02d", beijing_hour, current_time[4]);
    sprintf(new_date_str, "%04d/%d/%d", current_time[0], current_time[1], current_time[2]);
    if (strcmp(new_time_str, cached_time_str) != 0 || strcmp(new_date_str, cached_date_str) != 0)
    {
        strcpy(cached_time_str, new_time_str);
        strcpy(cached_date_str, new_date_str);
        lv_label_set_text(guider_ui.screen_label_time, cached_time_str);
        lv_label_set_text(guider_ui.screen_label_date, cached_date_str);
    }
}

// 天气更新定时器回调（每10分钟更新一次）
static uint32_t weather_timer_counter = 0;
static void weather_update_timer_cb(lv_timer_t * timer)
{
    weather_timer_counter++;
    
    // 每1分钟输出一次调试信息（600 * 100ms = 60秒）
    if (weather_timer_counter % 600 == 0)
    {
        printf("[定时器] 已过 %d 分钟，计数=%d\r\n", weather_timer_counter/600, weather_timer_counter);
    }
    // 10分钟 = 600秒，定时器周期100ms，所以计数到6000
    if (weather_timer_counter >= 6000)
    {
        weather_timer_counter = 0;
        printf("[定时器] 10分钟到达，开始更新天气...\r\n");
        ESP8266_GetWeather();  // 获取新数据
        update_weather_display();  // 更新显示
    }
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_FSMC_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  // 连接WiFi并获取网络时间
  ESP8266_Init();          // 连接 WiFi
  //ESP8266_GetTime();       // 获取网络时间

 
  //MyRTC_SetFromEpoch(time / 1000); // 转换为秒级
  
 ESP8266_GetWeather();
  // 启用LVGL图形界面
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  setup_ui(&guider_ui);
  events_init(&guider_ui);
  
  lv_timer_create(time_update_timer_cb, 1000, NULL);
  
  // 创建天气更新定时器（每100ms检查一次，每10分钟更新一次天气）
  lv_timer_create(weather_update_timer_cb, 100, NULL);
  
  
  
  // 在所有初始化完成后再启动TIM2定时器中断
  HAL_TIM_Base_Start_IT(&htim2);

  


 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    lv_timer_handler();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */