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

/* WiFi credentials (modify as needed) */
#define ESP_WIFI_SSID "8266"
#define ESP_WIFI_PASS "123456789"

#endif
