#include "esp8266.h"
#include "usart.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* simple ring buffer for received bytes */
static char rx_buf[4096];
static volatile int rx_len = 0;

//
void ESP8266_RxByte(uint8_t b)
{
  if (rx_len < (int)sizeof(rx_buf) - 1) {
    rx_buf[rx_len++] = (char)b;
    rx_buf[rx_len] = '\0';
  }
}

void ESP8266_UART_StartReceive(void)
{
  extern UART_HandleTypeDef huart2; /* from usart.c */
  static uint8_t dummy; /* placed in this file to start IT */
  HAL_UART_Receive_IT(&huart2, &dummy, 1);
}

void ESP8266_ClearBuf(void)
{
  rx_len = 0;
  rx_buf[0] = '\0';
}

int ESP8266_WaitFor(const char *token, uint32_t timeout_ms)
{
  uint32_t t = 0;
  while (t < timeout_ms) {
    if (token != NULL && strstr(rx_buf, token) != NULL) return 1;
    delay_ms(10);
    t += 10;
  }
  return 0;
}

int ESP8266_SendCommand(const char *cmd)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 1000);
  return 1;
}

/*
 * Performs simple HTTP GET via ESP8266 AT commands similar to your F1 code.
 * host: e.g. "api.pinduoduo.com"
 * http_request: full GET line(s) including Host and CRLFs, terminated with \r\n\r\n
 * Returns 1 on success and writes epoch into out_epoch if found, else 0.
 */
int ESP8266_GetEpochFromHttp(const char *host, const char *http_request, long long *out_epoch)
{
  char cmd[256];

  ESP8266_ClearBuf();
  /* start TCP connection */
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", host);
  ESP8266_SendCommand(cmd);
  if (!ESP8266_WaitFor("CONNECT", 5000) && !ESP8266_WaitFor("OK",2000)) return 0;

  /* set transparent mode */
  ESP8266_SendCommand("AT+CIPMODE=1\r\n");
  delay_ms(200);

  /* send CIPSEND to enter transparent mode */
  ESP8266_SendCommand("AT+CIPSEND\r\n");
  if (!ESP8266_WaitFor(">", 2000)) return 0;

  /* send HTTP request (in transparent mode) */
  ESP8266_ClearBuf();
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit(&huart2, (uint8_t*)http_request, strlen(http_request), 3000);
  delay_ms(1000);

  /* wait for server response accumulation */
  delay_ms(2000);

  /* exit transparent mode */
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit(&huart2, (uint8_t*)"+++", 3, 1000);
  delay_ms(1000);

  /* copy buffer locally for parsing */
  char local[2048];
  int copy_len = rx_len < (int)sizeof(local)-1 ? rx_len : (int)sizeof(local)-1;
  memcpy(local, rx_buf, copy_len);
  local[copy_len] = '\0';

  /* try to find "server_time" and parse number following it */
  char *p = strstr(local, "server_time");
  if (p) {
    long long epoch = 0;
    if (sscanf(p, "server_time\":%lld", &epoch) == 1 || sscanf(p, "server_time\": %lld", &epoch) == 1) {
      *out_epoch = epoch;
      return 1;
    }
    /* try looser parse */
    p = strstr(p, ":");
    if (p) {
      if (sscanf(p+1, "%lld", &epoch) == 1) { *out_epoch = epoch; return 1; }
    }
  }

  return 0;
}

/* ----------------- Migrated functions from your F1 code ----------------- */
/* Global variables similar to your original code (declared here) */
long long time = 0; /* received timestamp */
int weather = 0;//天气状态
int temperature = 0;//温度
int feels_like = 0;//体感温度
float wind_speed = 0.0f;//风速
int wind_direction = 0;//风向
int humidity = 0;//湿度
char weather_description[50] = {0};//天气状况文字
int current_weekday = 0;//星期（0=周日，1=周一，...，6=周六）
char weekday_str[10] = {0};//星期字符串

/* Helper to send string via huart2 */
// 发送字符串到ESP8266
static void esp_send(const char *s)
{
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), 2000);
}

void ESP8266_Init(void)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  
  // 首先尝试退出透传模式（防止上次异常退出）
  esp_send("+++");
  delay_ms(1000);
  ESP8266_ClearBuf();
  
  // 等待ESP8266就绪
  delay_ms(2000);

  /* 1. Test module (带重试) */
  int retry = 0;
  while (retry < 3) {
    ESP8266_ClearBuf();
    esp_send("AT\r\n");
    delay_ms(1000);
    if (strstr(rx_buf, "OK") != NULL) {
      printf("ESP8266通信正常\r\n");
      break;
    }
    retry++;
    printf("ESP8266通信测试 %d/3...\r\n", retry);
  }
  
  if (retry >= 3) {
    printf("ESP8266通信失败\r\n");
    return;
  }

  /* 2. Reset module */
  ESP8266_ClearBuf();
  esp_send("AT+RST\r\n");
  delay_ms(2000);

  /* 3. Set WiFi mode = station */
  ESP8266_ClearBuf();
  esp_send("AT+CWMODE=1\r\n");
  delay_ms(200);

  /* 4. Single connection mode */
  ESP8266_ClearBuf();
  esp_send("AT+CIPMUX=0\r\n");
  delay_ms(500);
  if (strstr(rx_buf, "OK") != NULL) printf("单连接模式正常\r\n");

  /* 5. Connect to AP */
  ESP8266_ClearBuf();
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ESP_WIFI_SSID, ESP_WIFI_PASS);
  esp_send(cmd);
  printf("发送连WiFi指令\r\n");

  uint8_t conn_flag = 0, ip_flag = 0;
  uint32_t start_time = 0;
  while (start_time < 10000) {
    if (strstr(rx_buf, "WIFI CONNECTED") != NULL) conn_flag = 1;
    if (strstr(rx_buf, "WIFI GOT IP") != NULL) ip_flag = 1;
    if (conn_flag && ip_flag) break;
    delay_ms(10);
    start_time += 10;
  }

  if (conn_flag && ip_flag) printf("WiFi连接成功\r\n");
  else printf("WiFi连接失败，超时/参数错误\r\n");
}

void ESP8266_GetTime(void)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  esp_send("AT+CWMODE=1\r\n");
  delay_ms(1500);
  printf("==========开始获取时间数据==========\r\n");
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", "api.pinduoduo.com");
  esp_send(cmd);
  printf("发送连接指令...\r\n");
  delay_ms(3500);

  if (strstr(rx_buf, "CONNECT") == NULL && strstr(rx_buf, "CONNECTOK") == NULL) {
    printf("连接失败\r\n");
    return;
  }

  /* set transparent */
  ESP8266_ClearBuf();
  esp_send("AT+CIPMODE=1\r\n");
  delay_ms(1000);

  /* start transparent send */
  ESP8266_ClearBuf();
  esp_send("AT+CIPSEND\r\n");
  printf("开始透传发送...\r\n");

  /* wait for ">" */
  uint32_t t = 0;
  while (strstr(rx_buf, ">") == NULL) {
    delay_ms(10);
    t += 10;
    if (t > 2000) { printf("进入透传模式失败\r\n"); return; }
  }
  printf("已进入透传模式\r\n");

  /* send HTTP request multiple times as in original */
  const char *http_request = "GET http://api.pinduoduo.com/api/server/_stm\r\n";
  for (int i = 0; i < 5; i++) {
    esp_send(http_request);
    printf("发送第%d次请求\r\n", i+1);
    delay_ms(500);
  }

  /* wait server response */
  printf("等待服务器响应...\r\n");
  delay_ms(3000);

  /* exit transparent mode */
  printf("准备退出透传模式...\r\n");
  delay_ms(1500);
  esp_send("+++");
  delay_ms(1500);

  /* copy response */
  char server_response[2048] = {0};
  int copy_len = rx_len < (int)sizeof(server_response)-1 ? rx_len : (int)sizeof(server_response)-1;
  memcpy(server_response, rx_buf, copy_len);
  server_response[copy_len] = '\0';

 char *data_start = strstr(server_response, ">");
    if(data_start != NULL) {
        data_start++; // 跳过'>'字符
        memmove(server_response, data_start, strlen(data_start)+1);
    }

  /* close */
  ESP8266_ClearBuf();
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(1000);

  printf("服务器返回数据：\r\n%s\r\n", server_response);

  /* parse JSON-like response for server_time */
  // 直接在整个响应中查找时间戳
  char *p = strstr(server_response, "server_time");
  if (p != NULL) {
    printf("找到server_time字段\r\n");
    
    // 查找冒号
    p = strchr(p, ':');
    if (p != NULL) {
      p++; // 跳过冒号
      
      // 跳过空格和引号
      while (*p == ' ' || *p == '"') p++;
      
      // 手动解析数字
      time = 0;
      while (*p >= '0' && *p <= '9') {
        time = time * 10 + (*p - '0');
        p++;
      }
      printf("手动解析的时间戳：%lld\r\n", time);
    } else {
      printf("未找到冒号分隔符\r\n");
      time = 0;
    }
  } else {
    printf("未找到server_time字段\r\n");
    time = 0;
  }
  printf("==========时间数据获取结束==========\r\n");
}

void ESP8266_GetWeather(void)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  printf("==========开始获取天气数据==========\r\n");
  
  // 确保退出透传模式并关闭现有连接
  printf("确保ESP8266处于正常状态...\r\n");
  // 多次发送+++确保退出透传模式
  esp_send("+++");
  delay_ms(500);
  esp_send("+++");
  delay_ms(500);
  esp_send("+++");
  delay_ms(1000);
  ESP8266_ClearBuf();
  
  // 关闭任何现有连接
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(1000);
  ESP8266_ClearBuf();
  
  // 确保处于非透传模式
  esp_send("AT+CIPMODE=0\r\n");
  delay_ms(500);
  ESP8266_ClearBuf();
  
  // 检查WiFi连接状态
  printf("检查WiFi连接状态...\r\n");
  esp_send("AT+CIPSTATUS\r\n");
  delay_ms(1000);
  printf("WiFi状态：%s\r\n", rx_buf);
  ESP8266_ClearBuf();
  
  char cmd[256];
  // 使用OpenWeatherMap HTTP API（支持体感温度和风速）
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", "api.openweathermap.org");
  esp_send(cmd);
  printf("正在连接服务器...\r\n");
  delay_ms(10000);

  if (strstr(rx_buf, "CONNECT") == NULL && strstr(rx_buf, "CONNECTOK") == NULL) {
    printf("服务器连接失败！返回数据：%s\r\n", rx_buf);
    return;
  }
  printf("服务器连接成功！\r\n");

  // 设置为透传模式
  ESP8266_ClearBuf();
  esp_send("AT+CIPMODE=1\r\n");
  delay_ms(500);
  if (strstr(rx_buf, "OK") == NULL) {
    printf("设置透传模式失败：%s\r\n", rx_buf);
    return;
  }
  printf("透传模式设置成功\r\n");
  ESP8266_ClearBuf();
  
  // 进入透传模式（带重试）
  int retry = 0;
  while (retry < 3) {
    esp_send("AT+CIPSEND\r\n");
    delay_ms(1500);
    if (strstr(rx_buf, ">") != NULL) {
      printf("已进入透传模式\r\n");
      break;
    }
    printf("进入透传模式失败，重试%d/3：%s\r\n", retry+1, rx_buf);
    ESP8266_ClearBuf();
    retry++;
  }
  
  if (retry >= 3) {
    printf("进入透传模式失败，返回非透传模式尝试\r\n");
    // 回退到非透传模式
    ESP8266_ClearBuf();
    // ... 这里可以添加非透传模式的代码
    return;
  }
  
  delay_ms(500);  // 等待一下确保">"已经接收
  ESP8266_ClearBuf();  // 清除">"和其他残留数据
  
  // 发送HTTP请求（透传模式下直接发送，不需要AT+CIPSEND）
  char http_req[512];
  int len = snprintf(http_req, sizeof(http_req), 
      "GET /data/2.5/weather?q=Hangzhou,CN&appid=9206757c5503e7d9e53ae6582464d2e2&units=metric&lang=zh_cn HTTP/1.1\r\n"
      "Host: api.openweathermap.org\r\n"
      "User-Agent: ESP8266/1.0\r\n"
      "Accept: application/json\r\n"
      "Connection: close\r\n\r\n");
  printf("HTTP请求内容：\r\n%s", http_req);
  printf("HTTP请求长度：%d\r\n", len);
  
  // 透传模式下直接发送数据
  esp_send(http_req);
  printf("HTTP请求发送完成！\r\n");
  
  // 等待服务器响应（分段检查，避免错过数据）
  printf("等待服务器响应（约10-15秒）...\r\n");
  int wait_time = 0;
  while (wait_time < 150) {  // 15秒
    delay_ms(100);
    wait_time++;
    // 检查是否收到HTTP响应头
    if (strstr(rx_buf, "HTTP/1.1") != NULL || strstr(rx_buf, "{\"coord\"") != NULL) {
      printf("收到服务器响应，等待%03dms\r\n", wait_time * 100);
      delay_ms(1000);  // 再等待1秒确保数据完整
      break;
    }
  }
  
  if (wait_time >= 150) {
    printf("等待超时，未收到服务器响应\r\n");
  }

  char server_response[2048] = {0};
  
  // 透传模式下，数据直接存放在rx_buf中
  int copy_len = rx_len < (int)sizeof(server_response)-1 ? rx_len : (int)sizeof(server_response)-1;
  memcpy(server_response, rx_buf, copy_len);
  server_response[copy_len] = '\0';
  
  printf("服务器返回数据（长度%d）：\r\n%s\r\n", rx_len, server_response);
  
  char *p;
  // OpenWeatherMap API解析
  // 返回JSON格式：{"main":{"temp":13.87,"feels_like":13.51},"wind":{"speed":1.84,"deg":78},"weather":[{"id":800,"main":"Clear"}]}
  
  // 解析温度 - 在"main"对象中查找"temp"
  p = strstr(server_response, "\"main\"");
  if (p) {
    char *main_section = p;
    p = strstr(main_section, "\"temp\":");
    if (p) {
      // 手动解析数字
      char *num_start = p + 7;
      while (*num_start && ((*num_start < '0' || *num_start > '9') && *num_start != '-' && *num_start != '.')) num_start++;
      
      // 提取数字字符串
      char num_str[20] = {0};
      int i = 0;
      while ((*num_start >= '0' && *num_start <= '9') || *num_start == '.' || *num_start == '-') {
        num_str[i++] = *num_start++;
        if (i >= 19) break;
      }
      num_str[i] = '\0';
      
      temperature = (int)atof(num_str);
    }
    
    // 在main对象中解析体感温度
    p = strstr(main_section, "\"feels_like\":");
    if (p) {
      char *num_start = p + 13;
      while (*num_start && ((*num_start < '0' || *num_start > '9') && *num_start != '-' && *num_start != '.')) num_start++;
      
      char num_str[20] = {0};
      int i = 0;
      while ((*num_start >= '0' && *num_start <= '9') || *num_start == '.' || *num_start == '-') {
        num_str[i++] = *num_start++;
        if (i >= 19) break;
      }
      num_str[i] = '\0';
      
      feels_like = (int)atof(num_str);
    }
    
    // 在main对象中解析湿度
    p = strstr(main_section, "\"humidity\":");
    if (p) {
      char *num_start = p + 11;
      while (*num_start && (*num_start < '0' || *num_start > '9')) num_start++;
      
      char num_str[10] = {0};
      int i = 0;
      while (*num_start >= '0' && *num_start <= '9') {
        num_str[i++] = *num_start++;
        if (i >= 9) break;
      }
      num_str[i] = '\0';
      
      humidity = atoi(num_str);
    }
  }
  
  // 解析风速 - 在"wind"对象中查找"speed"
  p = strstr(server_response, "\"wind\"");
  if (p) {
    char *wind_section = p;
    p = strstr(wind_section, "\"speed\":");
    if (p) {
      char *num_start = p + 8;
      while (*num_start && ((*num_start < '0' || *num_start > '9') && *num_start != '-' && *num_start != '.')) num_start++;
      
      char num_str[20] = {0};
      int i = 0;
      while ((*num_start >= '0' && *num_start <= '9') || *num_start == '.' || *num_start == '-') {
        num_str[i++] = *num_start++;
        if (i >= 19) break;
      }
      num_str[i] = '\0';
      
      // OpenWeatherMap返回的是m/s，转换为km/h
      wind_speed = atof(num_str) * 3.6f;
    }
    
    // 在wind对象中解析风向
    p = strstr(wind_section, "\"deg\":");
    if (p) {
      char *num_start = p + 6;
      while (*num_start && (*num_start < '0' || *num_start > '9')) num_start++;
      
      char num_str[10] = {0};
      int i = 0;
      while (*num_start >= '0' && *num_start <= '9') {
        num_str[i++] = *num_start++;
        if (i >= 9) break;
      }
      num_str[i] = '\0';
      
      wind_direction = atoi(num_str);
    }
  }
  
  // 解析天气代码 - 在"weather"数组中查找"id"
  p = strstr(server_response, "\"weather\"");
  if (p) {
    p = strstr(p, "\"id\":");
    if (p) {
      char *num_start = p + 5;
      while (*num_start && (*num_start < '0' || *num_start > '9')) num_start++;
      
      char num_str[10] = {0};
      int i = 0;
      while (*num_start >= '0' && *num_start <= '9') {
        num_str[i++] = *num_start++;
        if (i >= 9) break;
      }
      num_str[i] = '\0';
      
      weather = atoi(num_str);
    }
  }
  
  // 解析天气状况文字
  p = strstr(server_response, "\"description\":");
  if (p) {
    sscanf(p+15, "%49[^\"]", weather_description);
    printf("天气状况：%s\r\n", weather_description);
  }

  // 解析dt时间戳并计算星期
  p = strstr(server_response, "\"dt\":");
  if (p) {
    long timestamp = 0;
    sscanf(p+5, "%ld", &timestamp);
    if (timestamp > 0) {
      // 计算星期（0=周日，1=周一，...，6=周六）
      // 1970年1月1日是星期四，所以加4
      current_weekday = (int)((timestamp / 86400 + 4) % 7);
      
      // 转换为中文星期
      const char *weekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
      strcpy(weekday_str, weekdays[current_weekday]);
      printf("星期：%s\r\n", weekday_str);
    }
  }

  printf("解析结果：\r\n");
  printf("天气代码：%d\r\n", weather);
  printf("温度：%d°C\r\n", temperature);
  printf("体感温度：%d°C\r\n", feels_like);
  printf("湿度：%d%%\r\n", humidity);
  printf("风速：%.1f km/h\r\n", wind_speed);
  printf("风向：%d°\r\n", wind_direction);
  
  // 退出透传模式
  esp_send("+++");
  delay_ms(1000);
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(800);
  printf("==========天气数据获取结束==========\r\n");
}