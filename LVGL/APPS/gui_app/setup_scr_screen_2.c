/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_screen_2(lv_ui *ui)
{
    //Write codes screen_2
    ui->screen_2 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_2, 320, 240);
    lv_obj_set_scrollbar_mode(ui->screen_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_2, lv_color_hex(0xdcdcdc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_img_1
    ui->screen_2_img_1 = lv_img_create(ui->screen_2);
    lv_obj_add_flag(ui->screen_2_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_2_img_1, &_temputrue_alpha_64x50);
    lv_img_set_pivot(ui->screen_2_img_1, 50,50);
    lv_img_set_angle(ui->screen_2_img_1, 0);
    lv_obj_set_pos(ui->screen_2_img_1, 18, 90);
    lv_obj_set_size(ui->screen_2_img_1, 64, 50);

    //Write style for screen_2_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_2_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_2_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_2_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_label_1
    ui->screen_2_label_1 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_1, "8");
    lv_label_set_long_mode(ui->screen_2_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_1, 77, 108);
    lv_obj_set_size(ui->screen_2_label_1, 28, 25);

    //Write style for screen_2_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_1, &lv_font_Alibaba_PuHuiTi_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_2_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_2_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_img_2
    ui->screen_2_img_2 = lv_img_create(ui->screen_2);
    lv_obj_add_flag(ui->screen_2_img_2, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->screen_2_img_2, &_shidu_alpha_64x50);
    lv_img_set_pivot(ui->screen_2_img_2, 50,50);
    lv_img_set_angle(ui->screen_2_img_2, 0);
    lv_obj_set_pos(ui->screen_2_img_2, 166, 90);
    lv_obj_set_size(ui->screen_2_img_2, 64, 50);

    //Write style for screen_2_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_2_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_2_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_2_img_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_label_2
    ui->screen_2_label_2 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_2, "80");
    lv_label_set_long_mode(ui->screen_2_label_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_2, 241, 105);
    lv_obj_set_size(ui->screen_2_label_2, 52, 25);

    //Write style for screen_2_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_2, &lv_font_Alibaba_PuHuiTi_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_2_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_2_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_btn_1
    ui->screen_2_btn_1 = lv_btn_create(ui->screen_2);
    ui->screen_2_btn_1_label = lv_label_create(ui->screen_2_btn_1);
    lv_label_set_text(ui->screen_2_btn_1_label, "return");
    lv_label_set_long_mode(ui->screen_2_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_2_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_2_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_2_btn_1_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_2_btn_1, 252, 6);
    lv_obj_set_size(ui->screen_2_btn_1, 67, 27);

    //Write style for screen_2_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_2_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_2_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_2_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_2_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_btn_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_2_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_btn_1, &lv_font_Alibaba_PuHuiTi_Regular_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_2_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_2_label_3
    ui->screen_2_label_3 = lv_label_create(ui->screen_2);
    lv_label_set_text(ui->screen_2_label_3, "℃");
    lv_label_set_long_mode(ui->screen_2_label_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_2_label_3, 102, 108);
    lv_obj_set_size(ui->screen_2_label_3, 19, 17);

    //Write style for screen_2_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_2_label_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_2_label_3, &lv_font_Alibaba_PuHuiTi_Regular_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_2_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_2_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_2_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_2.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_2);

    //Init events for screen.
    events_init_screen_2(ui);
}
