/* Simple ESP8266 helper (HAL-based) - minimal API for HTTP GET and parsing
 * Provides interrupt-driven RX into an internal buffer and simple command helpers.
 */
#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>

void ESP8266_RxByte(uint8_t b);
void ESP8266_UART_StartReceive(void);
void ESP8266_ClearBuf(void);
int ESP8266_WaitFor(const char *token, uint32_t timeout_ms);
int ESP8266_SendCommand(const char *cmd);
int ESP8266_GetEpochFromHttp(const char *host, const char *http_request, long long *out_epoch);

/* High-level API (migrated from your STM32F1 code) */
void ESP8266_Init(void);
void ESP8266_GetTime(void); /* fills internal 'time' variable or returns via callback */
void ESP8266_GetWeather(void);

/* Weather data variables */
extern int weather;
extern int temperature;
extern int feels_like;
extern float wind_speed;
extern int wind_direction;
extern int humidity;
extern char weather_description[50];
extern char week_day[10];  // 星期几

/* WiFi credentials (modify as needed) */
#define ESP_WIFI_SSID "8266"
#define ESP_WIFI_PASS "123456789"

#define YOUR_API_KEY "9206757c5503e7d9e53ae6582464d2e2" //密钥
#define YOUR_LOCATION "Hangzhou" //杭州市

/* WiFi connection status */
extern volatile uint8_t wifi_connected;      // WiFi连接状态: 0=断开, 1=已连接
extern volatile uint8_t wifi_reconnect_count; // 重连计数

/* Reconnection functions */
int ESP8266_CheckConnection(void);           // 检查WiFi连接状态
int ESP8266_Reconnect(void);                // 断线重连
void ESP8266_ResetReconnectCount(void);     // 重置重连计数

/* MQTT functions */
#define MQTT_BROKER     "broker.hivemq.com"   // 公共MQTT服务器
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "STM32F407_Weather"
#define MQTT_TOPIC_PUB  "stm32/weather/data"   // 发布主题
#define MQTT_TOPIC_SUB  "stm32/weather/cmd"    // 订阅主题

int ESP8266_MQTT_Init(void);                 // 初始化MQTT连接
int ESP8266_MQTT_Publish(const char *data);  // 发布消息
int ESP8266_MQTT_Subscribe(void);            // 订阅主题
int ESP8266_MQTT_CheckMessage(char *topic, char *message, int max_len); // 检查接收消息
void ESP8266_MQTT_Disconnect(void);          // 断开MQTT连接

#endif
