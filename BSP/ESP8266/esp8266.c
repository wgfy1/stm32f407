#include "esp8266.h"
#include "usart.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/* simple ring buffer for received bytes */
static char rx_buf[4096];
static volatile int rx_len = 0;

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
int weather = 0;
int temperature = 0;
int wind_direction = 0;

/* Helper to send string via huart2 */
static void esp_send(const char *s)
{
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), 2000);
}

void ESP8266_Init(void)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  delay_ms(1000);

  /* 1. Test module */
  esp_send("AT\r\n");
  delay_ms(500);
  if (strstr(rx_buf, "OK") != NULL) {
    printf("ESP8266通信正常\r\n");
  } else {
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
  delay_ms(2000);
  printf("==========开始获取时间数据==========\r\n");
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", "api.pinduoduo.com");
  esp_send(cmd);
  printf("发送连接指令...\r\n");
  delay_ms(5500);

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

  /* close */
  ESP8266_ClearBuf();
  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(1000);

  printf("服务器返回数据：\r\n%s\r\n", server_response);

  /* parse JSON-like response for server_time */
  char *start = strstr(server_response, "{");
  char *end = strstr(server_response, "}");
  if (start != NULL && end != NULL) {
    char json_buffer[512] = {0};
    int len = end - start + 1;
    if (len < (int)sizeof(json_buffer)) {
      memcpy(json_buffer, start, len);
      printf("完整JSON响应: %s\r\n", json_buffer);
      char *p = strstr(json_buffer, "server_time");
      if (p != NULL) {
        if (sscanf(p+12, "%lld", &time) == 1) {
          printf("提取的服务器时间戳：%lld\r\n", time);
        }
      }
    }
  } else {
    printf("未找到有效时间戳数据\r\n");
    time = 0;
  }
  printf("==========时间数据获取结束==========\r\n");
}

void ESP8266_GetWeather(void)
{
  ESP8266_ClearBuf();
  ESP8266_UART_StartReceive();
  printf("==========开始获取天气数据==========\r\n");
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", "api.seniverse.com");
  esp_send(cmd);
  delay_ms(2000);

  if (strstr(rx_buf, "CONNECT") == NULL && strstr(rx_buf, "CONNECTOK") == NULL) {
    printf("服务器连接失败！返回数据：%s\r\n", rx_buf);
    return;
  }
  printf("服务器连接成功！\r\n");

  ESP8266_ClearBuf();
  esp_send("AT+CIPMODE=1\r\n");
  delay_ms(1000);

  ESP8266_ClearBuf();
  esp_send("AT+CIPSEND\r\n");
  delay_ms(200);

  uint32_t t = 0;
  while (strstr(rx_buf, ">") == NULL) {
    delay_ms(10);
    t += 10;
    if (t > 2000) { printf("进入透传模式失败\r\n"); return; }
  }
  printf("已进入透传模式\r\n");

  const char *http_req = "GET /v3/weather/now.json?key=S1p_vFyFaY1XbmB4q&location=hangzhou&language=en&unit=c HTTP/1.1\r\nHost: api.seniverse.com\r\nConnection: close\r\n\r\n";
  esp_send(http_req);
  printf("HTTP请求发送完成！\r\n");
  delay_ms(3000);

  printf("准备退出透传模式...\r\n");
  delay_ms(1500);
  esp_send("+++");
  delay_ms(1500);

  char server_response[2048] = {0};
  int copy_len = rx_len < (int)sizeof(server_response)-1 ? rx_len : (int)sizeof(server_response)-1;
  memcpy(server_response, rx_buf, copy_len);
  server_response[copy_len] = '\0';

  /* parse basic fields */
  char *p;
  p = strstr(server_response, "code");
  if (p) sscanf(p+7, "%d", &weather);
  p = strstr(server_response, "temperature");
  if (p) sscanf(p+14, "%d", &temperature);
  p = strstr(server_response, "wind_direction");
  if (p) sscanf(p+15, "%d", &wind_direction);

  esp_send("AT+CIPCLOSE\r\n");
  delay_ms(800);
  printf("==========天气数据获取结束==========\r\n");
}

