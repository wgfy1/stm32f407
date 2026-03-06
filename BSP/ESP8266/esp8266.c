#include "esp8266.h"
#include "usart.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* simple ring buffer for received bytes */
static char rx_buf[4096];// 接收缓冲区
static volatile int rx_len = 0;// 接收数据长度

void ESP8266_RxByte(uint8_t b)// 接收字节
{
  if (rx_len < (int)sizeof(rx_buf) - 1) {
    rx_buf[rx_len++] = (char)b;
    rx_buf[rx_len] = '\0';
  }
}

void ESP8266_UART_StartReceive(void)// 启动UART接收中断
{
  extern UART_HandleTypeDef huart2; // 使用UART2
  static uint8_t dummy; // 用于启动UART接收中断的dummy字节
  HAL_UART_Receive_IT(&huart2, &dummy, 1);// 启动UART接收中断
}

void ESP8266_ClearBuf(void)// 清空接收缓冲区
{
  rx_len = 0;
  rx_buf[0] = '\0';
}

int ESP8266_WaitFor(const char *token, uint32_t timeout_ms)// 等待特定字符串出现，带超时
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
char week_day[10] = "--";//星期几，默认为--

/* WiFi connection status for reconnection mechanism */
volatile uint8_t wifi_connected = 0;       // WiFi连接状态: 0=断开, 1=已连接
volatile uint8_t wifi_reconnect_count = 0; // 连续重连失败计数
#define MAX_RECONNECT_ATTEMPTS 5           // 最大重连尝试次数

// 根据时间戳计算星期几
static void calculate_weekday(long long timestamp)
{
    // Unix时间戳转星期几 (Zeller公式简化版)
    // 1970年1月1日是星期四
    int days_since_1970 = (int)(timestamp / 86400);
    int week = (days_since_1970 + 4) % 7;  // 0=周日, 1=周一, ...
    
    switch(week) {
        case 0: strcpy(week_day, "星期日"); break;
        case 1: strcpy(week_day, "星期一"); break;
        case 2: strcpy(week_day, "星期二"); break;
        case 3: strcpy(week_day, "星期三"); break;
        case 4: strcpy(week_day, "星期四"); break;
        case 5: strcpy(week_day, "星期五"); break;
        case 6: strcpy(week_day, "星期六"); break;
        default: strcpy(week_day, "-"); break;
    }
}

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

  /* 查看AT固件版本 */
  ESP8266_ClearBuf();
  esp_send("AT+GMR\r\n");
  delay_ms(1000);
  printf("[ESP8266] 固件版本: %s\r\n", rx_buf);
  
  /* 查看支持的MQTT指令 */
  ESP8266_ClearBuf();
  esp_send("AT+MQTT?\r\n");
  delay_ms(1000);
  printf("[ESP8266] MQTT指令支持: %s\r\n", rx_buf);
  
  /* 查看所有支持的AT指令 */
  ESP8266_ClearBuf();
  esp_send("AT+CMD?\r\n");
  delay_ms(2000);
  printf("[ESP8266] 支持的AT指令: %s\r\n", rx_buf);

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

  if (conn_flag && ip_flag) {
    printf("WiFi连接成功\r\n");
    wifi_connected = 1;       // 设置连接状态
    wifi_reconnect_count = 0; // 重置重连计数
  } else {
    printf("WiFi连接失败，超时/参数错误\r\n");
    wifi_connected = 0;
  }
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
  delay_ms(500);
  
  // 1. 连接天气服务器TCP，带重试机制
  int server_retry = 0;
  int connected = 0;
  char cmd[256];// 用于构建AT命令的缓冲区
  
  while (server_retry < 3 && !connected) {
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", "api.openweathermap.org");// 构建连接命令
    esp_send(cmd);// 发送连接命令
    printf("正在连接天气服务器... (尝试 %d/3)\r\n", server_retry + 1);
    
    // 等待连接结果
    int connect_wait = 0;
    while (connect_wait < 50) {  // 5秒超时
      delay_ms(100);
      connect_wait++;
      if (strstr(rx_buf, "CONNECT") != NULL || strstr(rx_buf, "CONNECTOK") != NULL) {
        connected = 1;
        break;
      }
      if (strstr(rx_buf, "ERROR") != NULL || strstr(rx_buf, "CLOSED") != NULL) {
        break;  // 连接失败
      }
    }
    
    if (!connected) {
      printf("服务器连接失败！返回：%s\r\n", rx_buf);
      ESP8266_ClearBuf();
      esp_send("AT+CIPCLOSE\r\n");
      delay_ms(500);
      ESP8266_ClearBuf();
      server_retry++;
      if (server_retry < 3) {
        printf("等待2秒后重试...\r\n");
        delay_ms(2000);
      }
    }
  }
  if (!connected) {
    printf("服务器连接失败，已重试3次\r\n");
    // 检查WiFi连接状态，尝试断线重连
    if (!ESP8266_CheckConnection()) {
      printf("[ESP8266] 检测到WiFi断开，尝试重连...\r\n");
      if (ESP8266_Reconnect()) {
        printf("[ESP8266] 重连成功，下次定时器将重新获取天气\r\n");
      } else {
        printf("[ESP8266] 重连失败，请检查网络环境\r\n");
      }
    }
    return;
  }
  printf("服务器连接成功！\r\n");

  // 设置为透传模式
  ESP8266_ClearBuf();
  esp_send("AT+CIPMODE=1\r\n");
  delay_ms(1000);
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

  delay_ms(800);  // 等待一下确保">"已经接收
  ESP8266_ClearBuf();  // 清除">"和其他残留数据
  
  // 3. 发送天气请求（透传模式下直接发送）
  char http_req[512];
  int len = snprintf(http_req, sizeof(http_req), 
      "GET /data/2.5/weather?q=%s,CN&appid=%s&units=metric&lang=zh_cn HTTP/1.1\r\n"
      "Host: api.openweathermap.org\r\n"
      "User-Agent: ESP8266/1.0\r\n"
      "Accept: application/json\r\n"
      "Connection: close\r\n\r\n",
      YOUR_LOCATION, YOUR_API_KEY);
  printf("HTTP请求内容：\r\n%s", http_req);
  printf("HTTP请求长度：%d\r\n", len);
  
  // 透传模式下直接发送数据
  esp_send(http_req);
  printf("HTTP请求发送完成！\r\n");
  
  // 等待服务器响应（分段检查，避免错过数据）
  printf("等待服务器响应（约10-20秒）...\r\n");
  int wait_time = 0;
  while (wait_time < 200) {  // 15秒
    delay_ms(1000);
    wait_time++;
    // 检查是否收到HTTP响应头
    if (strstr(rx_buf, "HTTP/1.1") != NULL || strstr(rx_buf, "{\"coord\"") != NULL) {
      printf("收到服务器响应，等待%03dms\r\n", wait_time * 100);
      delay_ms(1000);  // 再等待1秒确保数据完整
      break;
    }
  } 
  if (wait_time >= 200) {
    printf("等待超时，未收到服务器响应\r\n");
  }
  char server_response[2048] = {0};
  
  // 透传模式下，数据直接存放在rx_buf中
  int copy_len = rx_len < (int)sizeof(server_response)-1 ? rx_len : (int)sizeof(server_response)-1;
  memcpy(server_response, rx_buf, copy_len);
  server_response[copy_len] = '\0';
  
 // printf("服务器返回数据（长度%d）：\r\n%s\r\n", rx_len, server_response);

  
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
  
  // 解析天气API中的时间戳 "dt"
  p = strstr(server_response, "\"dt\":");
  if (p) {
    char *num_start = p + 5;
    while (*num_start && (*num_start < '0' || *num_start > '9')) num_start++;
    
    long long timestamp = 0;
    while (*num_start >= '0' && *num_start <= '9') {
      timestamp = timestamp * 10 + (*num_start - '0');
      num_start++;
    }
    if (timestamp > 0) {
      time = timestamp;  // 更新全局时间变量
      calculate_weekday(timestamp);
      printf("时间戳：%lu, 星期：%s\r\n", (unsigned long)timestamp, week_day);
    }
  } else if (time > 0) {
    // 如果没有dt字段但time已有值，使用已有值
    // 不再重复计算，直接使用已有的week_day
    printf("使用已有时间，星期：%s\r\n", week_day);
  }

  printf("解析结果：\r\n");
  printf("天气代码：%d\r\n", weather);
  printf("温度：%d°C\r\n", temperature);
  printf("体感温度：%d°C\r\n", feels_like);
  printf("湿度：%d%%\r\n", humidity);
  printf("风速：%.1f km/h\r\n", wind_speed);
  printf("风向：%d°\r\n", wind_direction);
  printf("星期：%s\r\n", week_day);
  
  // 4. 退出透传模式
  printf("退出透传模式\r\n");
  delay_ms(1500);
  esp_send("+++");
  delay_ms(2000);
  
  // 5. 关闭天气服务器TCP连接
  ESP8266_ClearBuf();
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(2000);  // 增加延时，确保连接完全关闭
  printf("==========天气数据获取结束==========\r\n");
  
  // 6. 单连接模式下HTTP会断开MQTT，需要重新初始化MQTT
  printf("[MQTT] HTTP请求完成，等待模块就绪...\r\n");
  delay_ms(3000);  // 等待ESP8266稳定
  
  // 测试AT指令，确保模块就绪
  int at_retry = 3;
  while (at_retry > 0) {
    ESP8266_ClearBuf();
    esp_send("AT\r\n");
    if (ESP8266_WaitFor("OK", 2000)) {
      printf("[MQTT] 模块就绪，开始连接MQTT...\r\n");
      break;
    }
    delay_ms(1000);
    at_retry--;
  }
  
  ESP8266_MQTT_Init();
}

/**
  * @brief  检查WiFi连接状态
  * @retval 1=已连接, 0=断开
  */
int ESP8266_CheckConnection(void)
{
  ESP8266_ClearBuf();
  esp_send("AT+CIPSTATUS\r\n");
  delay_ms(1000);
  
  // 如果返回"STATUS:2"或"STATUS:3"或"STATUS:4"表示已连接
  // 如果返回"STATUS:5"表示断开
  if (strstr(rx_buf, "STATUS:5") != NULL || strstr(rx_buf, "ERROR") != NULL) {
    wifi_connected = 0;
    printf("[ESP8266] WiFi已断开\r\n");
    return 0;
  }
  
  wifi_connected = 1;
  return 1;
}

/**
  * @brief  断线重连函数
  * @retval 1=重连成功, 0=重连失败
  */
int ESP8266_Reconnect(void)
{
  printf("[ESP8266] 开始断线重连... (第%d次)\r\n", wifi_reconnect_count + 1);
  
  // 检查是否超过最大重连次数
  if (wifi_reconnect_count >= MAX_RECONNECT_ATTEMPTS) {
    printf("[ESP8266] 已达最大重连次数(%d)，停止重连\r\n", MAX_RECONNECT_ATTEMPTS);
    return 0;
  }
  
  wifi_reconnect_count++;
  
  // 1. 先尝试退出透传模式
  delay_ms(1000);
  esp_send("+++");
  delay_ms(1500);
  
  // 2. 关闭所有连接
  ESP8266_ClearBuf();
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(500);
  
  // 3. 检查WiFi是否还连接
  ESP8266_ClearBuf();
  esp_send("AT+CWJAP?\r\n");
  delay_ms(2000);
  
  if (strstr(rx_buf, "+CWJAP:") != NULL) {
    // WiFi还连接着，只是TCP断开了
    printf("[ESP8266] WiFi仍连接，TCP已断开，重新连接成功\r\n");
    wifi_connected = 1;
    wifi_reconnect_count = 0;  // 重置计数
    return 1;
  }
  
  // 4. WiFi断开了，需要重新连接
  printf("[ESP8266] WiFi已断开，尝试重新连接...\r\n");
  ESP8266_ClearBuf();
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ESP_WIFI_SSID, ESP_WIFI_PASS);
  esp_send(cmd);
  
  // 等待连接结果（最多15秒）
  uint8_t conn_flag = 0, ip_flag = 0;
  uint32_t start_time = 0;
  while (start_time < 15000) {
    if (strstr(rx_buf, "WIFI CONNECTED") != NULL) conn_flag = 1;
    if (strstr(rx_buf, "WIFI GOT IP") != NULL) ip_flag = 1;
    if (strstr(rx_buf, "FAIL") != NULL || strstr(rx_buf, "ERROR") != NULL) {
      printf("[ESP8266] 重连失败\r\n");
      break;
    }
    if (conn_flag && ip_flag) break;
    delay_ms(100);
    start_time += 100;
  }
  
  if (conn_flag && ip_flag) {
    printf("[ESP8266] WiFi重连成功\r\n");
    wifi_connected = 1;
    wifi_reconnect_count = 0;  // 重置计数
    return 1;
  } else {
    printf("[ESP8266] WiFi重连失败\r\n");
    wifi_connected = 0;
    return 0;
  }
}

/**
  * @brief  重置重连计数
  */
void ESP8266_ResetReconnectCount(void)
{
  wifi_reconnect_count = 0;
}

/* ===================== MQTT Functions ===================== */

/**
  * @brief  初始化MQTT连接
  * @retval 0:失败, 1:成功
  */
int ESP8266_MQTT_Init(void)
{
  char cmd[256];
  
  printf("[MQTT] 初始化MQTT连接...\r\n");
  
  // 0. 先测试AT指令
  ESP8266_ClearBuf();
  esp_send("AT\r\n");
  if (!ESP8266_WaitFor("OK", 2000)) {
    printf("[MQTT] AT指令无响应\r\n");
    return 0;
  }
  
  // 1. 设置MQTT用户配置 (scheme=1表示MQTT over TCP)
  snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"%s\",\"\",\"\",0,0,\"\"\r\n", MQTT_CLIENT_ID);
  printf("[MQTT] 用户配置指令: %s", cmd);
  ESP8266_ClearBuf();
  esp_send(cmd);
  if (!ESP8266_WaitFor("OK", 5000)) {
    printf("[MQTT] 用户配置失败，尝试继续连接...\r\n");
    // 不返回错误，继续尝试连接
  }
  delay_ms(200);
  
  // 2. 连接MQTT服务器
  snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,1\r\n", MQTT_BROKER, MQTT_PORT);
  printf("[MQTT] 连接指令: %s", cmd);
  ESP8266_ClearBuf();
  esp_send(cmd);
  if (!ESP8266_WaitFor("OK", 10000)) {
    printf("[MQTT] 连接失败，返回: %s\r\n", rx_buf);
    return 0;
  }
  
  printf("[MQTT] 连接成功\r\n");
  
  // 3. 订阅命令主题
  ESP8266_MQTT_Subscribe();
  
  return 1;
}

/**
  * @brief  发布天气数据到MQTT
  * @param  data: JSON格式的数据
  * @retval 0:失败, 1:成功
  */

int ESP8266_MQTT_Publish(const char *data)
{
  char cmd[512];
  
  printf("[MQTT] 准备发布数据: %s\r\n", data);
  
  // 使用TCP透传方式发送MQTT消息
  // 1. 建立TCP连接
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", MQTT_BROKER, MQTT_PORT);
  printf("[MQTT] TCP连接指令: %s", cmd);
  ESP8266_ClearBuf();
  esp_send(cmd);
  if (!ESP8266_WaitFor("CONNECT", 5000)) {
    printf("[MQTT] TCP连接失败\r\n");
    return 0;
  }
  printf("[MQTT] TCP连接成功\r\n");
  
  // 2. 进入透传模式
  ESP8266_ClearBuf();
  esp_send("AT+CIPMODE=1\r\n");
  if (!ESP8266_WaitFor("OK", 2000)) {
    printf("[MQTT] 设置透传模式失败\r\n");
    esp_send("AT+CIPCLOSE\r\n");
    return 0;
  }
  
  // 3. 开始透传
  ESP8266_ClearBuf();
  esp_send("AT+CIPSEND\r\n");
  delay_ms(500);
  
  // 4. 构建并发送MQTT CONNECT消息
  unsigned char connect_msg[128];
  int connect_len = 0;
  int payload_len = 0;
  
  // 先计算payload长度
  char *client_id = "STM32F407";
  int client_id_len = strlen(client_id);
  payload_len = 2 + 4 + 1 + 1 + 2 + 2 + client_id_len; // 协议名长度 + 协议名 + 协议级别 + 连接标志 + 保持连接 + 客户端ID长度 + 客户端ID
  
  // 固定头
  connect_msg[connect_len++] = 0x10; // CONNECT
  connect_msg[connect_len++] = payload_len; // 剩余长度
  
  // 协议名
  connect_msg[connect_len++] = 0x00;
  connect_msg[connect_len++] = 0x04;
  connect_msg[connect_len++] = 'M';
  connect_msg[connect_len++] = 'Q';
  connect_msg[connect_len++] = 'T';
  connect_msg[connect_len++] = 'T';
  
  // 协议级别
  connect_msg[connect_len++] = 0x04;
  
  // 连接标志
  connect_msg[connect_len++] = 0x02; // Clean session
  
  // 保持连接时间
  connect_msg[connect_len++] = 0x00;
  connect_msg[connect_len++] = 0x3C; // 60秒
  
  // 客户端ID
  connect_msg[connect_len++] = 0x00;
  connect_msg[connect_len++] = client_id_len;
  memcpy(&connect_msg[connect_len], client_id, client_id_len);
  connect_len += client_id_len;
  
  // 发送CONNECT消息
  extern UART_HandleTypeDef huart2;
  printf("[MQTT] 发送CONNECT消息，长度: %d，payload: %d\r\n", connect_len, payload_len);
  HAL_UART_Transmit(&huart2, connect_msg, connect_len, 1000);
  
  // 等待CONNACK响应
  delay_ms(1500);
  printf("[MQTT] 等待CONNACK...\r\n");
  
  // 5. 构建并发送MQTT PUBLISH消息
  unsigned char publish_msg[512];
  int publish_len = 0;
  
  // 固定头
  publish_msg[publish_len++] = 0x30; // PUBLISH
  
  // 主题长度
  int topic_len = strlen(MQTT_TOPIC_PUB);
  int data_len = strlen(data);
  int remaining_len = 2 + topic_len + data_len;
  
  publish_msg[publish_len++] = remaining_len;
  
  // 主题
  publish_msg[publish_len++] = (topic_len >> 8) & 0xFF;
  publish_msg[publish_len++] = topic_len & 0xFF;
  memcpy(&publish_msg[publish_len], MQTT_TOPIC_PUB, topic_len);
  publish_len += topic_len;
  
  // 消息内容
  memcpy(&publish_msg[publish_len], data, data_len);
  publish_len += data_len;
  
  // 发送PUBLISH消息
  printf("[MQTT] 发送PUBLISH消息，长度: %d\r\n", publish_len);
  HAL_UART_Transmit(&huart2, publish_msg, publish_len, 1000);
  
  // 等待一段时间确保消息发送完成
  delay_ms(2000);
  printf("[MQTT] PUBLISH消息已发送\r\n");
  
  // 6. 退出透传模式
  esp_send("+++");
  delay_ms(1500);
  
  // 7. 关闭连接
  ESP8266_ClearBuf();
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(1000);
  
  printf("[MQTT] 发布完成\r\n");
  return 1;
}

/**
  * @brief  订阅命令主题
  * @retval 0:失败, 1:成功
  */
int ESP8266_MQTT_Subscribe(void)
{
  char cmd[256];
  
  snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",1\r\n", MQTT_TOPIC_SUB);
  printf("[MQTT] 订阅指令: %s", cmd);
  ESP8266_ClearBuf();
  esp_send(cmd);
  
  if (ESP8266_WaitFor("OK", 5000)) {
    printf("[MQTT] 订阅成功: %s\r\n", MQTT_TOPIC_SUB);
    return 1;
  } else {
    printf("[MQTT] 订阅失败，返回: %s\r\n", rx_buf);
    return 0;
  }
}

/**
  * @brief  检查是否收到MQTT消息
  * @param  topic: 输出主题缓冲区
  * @param  message: 输出消息缓冲区
  * @param  max_len: 缓冲区最大长度
  * @retval 0:无消息, 1:收到消息
  */
int ESP8266_MQTT_CheckMessage(char *topic, char *message, int max_len)
{
  // 检查接收缓冲区中是否有MQTT消息
  // 格式: +MQTTSUBRECV:0,"topic",len,"message"
  char *p = strstr(rx_buf, "+MQTTSUBRECV:");
  if (p) {
    // 解析主题和消息
    char *topic_start = strchr(p, '"');
    if (topic_start) {
      topic_start++;
      char *topic_end = strchr(topic_start, '"');
      if (topic_end) {
        int topic_len = topic_end - topic_start;
        if (topic_len < max_len) {
          memcpy(topic, topic_start, topic_len);
          topic[topic_len] = '\0';
        }
        
        // 查找消息内容
        char *msg_start = strchr(topic_end + 1, '"');
        if (msg_start) {
          msg_start++;
          char *msg_end = strrchr(msg_start, '"');
          if (msg_end) {
            int msg_len = msg_end - msg_start;
            if (msg_len < max_len) {
              memcpy(message, msg_start, msg_len);
              message[msg_len] = '\0';
            }
            
            // 清除已处理的消息
            ESP8266_ClearBuf();
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

/**
  * @brief  断开MQTT连接
  */
void ESP8266_MQTT_Disconnect(void)
{
  esp_send("AT+MQTTDISCONN=0\r\n");
  ESP8266_WaitFor("OK", 3000);
  printf("[MQTT] 已断开连接\r\n");
}