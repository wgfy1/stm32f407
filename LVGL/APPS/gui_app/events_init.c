/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "rtc.h"
#include "esp8266.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

// 天气数据更新标志（在main.c中定义）
extern volatile uint8_t weather_update_flag;

// 天气数据缓存
extern int temperature;
extern int feels_like;
extern float wind_speed;
extern int wind_direction;
extern int weather;
extern char weather_description[50];

// 更新天气显示
void update_weather_display(void)
{
    char temp_str[20];
    char feels_str[20];
    char wind_str[20];
    char humidity_str[20];
    
    // 格式化温度（带单位）
    sprintf(temp_str, "%d", temperature);
    lv_label_set_text(guider_ui.screen_3_label_temperature, temp_str);
    
    // 格式化体感温度（带单位）
    sprintf(feels_str, "%d°", feels_like);
    lv_label_set_text(guider_ui.screen_3_label_feels_like, feels_str);
    
    // 格式化风速（带单位）
    sprintf(wind_str, "%.1fkm/h", wind_speed);
    lv_label_set_text(guider_ui.screen_3_label_wind_speed, wind_str);
    
    // 格式化湿度（带单位）
    sprintf(humidity_str, "%d%%", humidity);
    lv_label_set_text(guider_ui.screen_3_label_humidity, humidity_str);
    
    // 显示天气状况
    lv_label_set_text(guider_ui.screen_3_label_code, weather_description);
    
    printf("天气显示已更新：%s, 温度%s, 体感%s, 风速%s\r\n", weather_description, temp_str, feels_str, wind_str);
}


static void screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {
        extern uint16_t current_time[7];
        MyRTC_ReadTimeToArray(current_time);
        uint8_t beijing_hour = current_time[3] + 8;
        if (beijing_hour >= 24)
        {
            beijing_hour -= 24;
        }
        char time_str[10], date_str[40];
        sprintf(time_str, "%02d:%02d", beijing_hour, current_time[4]);
        sprintf(date_str, "%04d/%d/%d,%s", current_time[0], current_time[1], current_time[2], week_day);
        lv_label_set_text(guider_ui.screen_label_time, time_str);
        lv_label_set_text(guider_ui.screen_label_date, date_str);
        break;
    }
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_LEFT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, guider_ui.screen_1_del, &guider_ui.screen_del, setup_scr_screen_1, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, true, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void events_init_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen, screen_event_handler, LV_EVENT_ALL, ui);
}

static void screen_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_1_del, setup_scr_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, true, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void screen_1_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_2, guider_ui.screen_2_del, &guider_ui.screen_1_del, setup_scr_screen_2, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_1_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_3, guider_ui.screen_3_del, &guider_ui.screen_1_del, setup_scr_screen_3, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_1 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_1, screen_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_1_btn_1, screen_1_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_1_btn_2, screen_1_btn_2_event_handler, LV_EVENT_ALL, ui);
}

static void screen_2_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, guider_ui.screen_1_del, &guider_ui.screen_2_del, setup_scr_screen_1, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_2 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_2_btn_1, screen_2_btn_1_event_handler, LV_EVENT_ALL, ui);
}

static void screen_3_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, guider_ui.screen_1_del, &guider_ui.screen_3_del, setup_scr_screen_1, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

static void screen_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCREEN_LOADED:
    {
        // 屏幕加载时更新天气显示
        update_weather_display();
        break;
    }
    default:
        break;
    }
}

void events_init_screen_3 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_3, screen_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_3_btn_1, screen_3_btn_1_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
