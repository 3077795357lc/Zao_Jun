#include <stdio.h>
#include <unistd.h>
#include "../usrcode/data/dev_status.h"
#include "../usrcode/hardware/gpio.h"

#include "lvgl/lvgl.h"
#include "lvgl/drivers/display/lv_linux_fbdev.h"
#include "lvgl/drivers/indev/lv_evdev.h"

static void led_btn_click_cb(lv_event_t *e);
static void air_btn_click_cb(lv_event_t *e);

// 设备状态文本 label
static lv_obj_t *led_label;
static lv_obj_t *air_label;

// 开关按钮
static lv_obj_t *led_btn;
static lv_obj_t *air_btn;

/* 32px 中文黑体，定义见 fonts/heiti_32.c */
LV_FONT_DECLARE(heiti_32);

/*
 * 周期定时器回调：每 50ms 读取一次设备状态表，
 * 把状态同步到两个按钮上的 ON/OFF 文本。
 */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;  

    if (DEVICE_STATUS_ON == get_dev_status(DEVICE_LIGHT)) {
        lv_label_set_text(led_label, "ON");
    } else {
        lv_label_set_text(led_label, "OFF");
    }

    if (DEVICE_STATUS_ON == get_dev_status(DEVICE_AIRCONDITIONER)) {
        lv_label_set_text(air_label, "ON");
    } else {
        lv_label_set_text(air_label, "OFF");
    }
}

/* 创建 UI */
static void ui_create(void)
{
    lv_obj_t *scr = lv_scr_act();   /* 当前活动屏幕，新控件挂在下面 */

    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);   //白底

    // 灯具、空调两个开关按钮，分别位于底部左右
    led_btn = lv_button_create(scr);
    air_btn = lv_button_create(scr);

    lv_obj_set_size(led_btn, 200, 100);
    lv_obj_set_size(air_btn, 200, 100);

    lv_obj_align(led_btn, LV_ALIGN_BOTTOM_LEFT, 0, -30);
    lv_obj_align(air_btn, LV_ALIGN_BOTTOM_RIGHT, 0, -30);

    // 默认显示 OFF 
    led_label = lv_label_create(led_btn);
    lv_label_set_text(led_label, "OFF");
    lv_obj_set_style_text_font(led_label, &heiti_32, 0);
    lv_obj_center(led_label);

    air_label = lv_label_create(air_btn);
    lv_label_set_text(air_label, "OFF");
    lv_obj_set_style_text_font(air_label, &heiti_32, 0);
    lv_obj_center(air_label);

    // 点击按钮可以开关对应设备并更新状态表
    lv_obj_add_event_cb(led_btn, led_btn_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(air_btn, air_btn_click_cb, LV_EVENT_CLICKED, NULL);

    // 每 50ms 触发一次ui_timer_cb
    lv_timer_create(ui_timer_cb, 50, NULL);
}

// 灯具开关
static void led_btn_click_cb(lv_event_t *e)
{
    (void)e;  

    led_ctrl();
    set_dev_status(DEVICE_LIGHT);
}

//空调开关
static void air_btn_click_cb(lv_event_t *e)
{
    (void)e;   

    air_con_ctrl();
    set_dev_status(DEVICE_AIRCONDITIONER);
}

int main(void)
{
    // 硬件与 LVGL 环境初始化
    air_con_init();
    led_init();
    lv_init();
    dev_status_init();

    // 显示后端：打开 /dev/fb0，读取分辨率/色深并 mmap 显存
    lv_display_t *disp = lv_linux_fbdev_create();
    if (lv_linux_fbdev_set_file(disp, "/dev/fb0") != LV_RESULT_OK) {
        printf("fbdev /dev/fb0 init failed\n");
        return 1;
    }

    // 触摸输入：goodix-ts 触屏设备对应 /dev/input/event1
    lv_indev_t *ts = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event1");
    if (ts == NULL) {
        printf("evdev /dev/input/event1 init failed\n");
    }

    // 创建 UI 
    ui_create();

    // LVGL 引擎主循环：执行到期定时器、按需重绘屏幕
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY)
            sleep_ms = LV_DEF_REFR_PERIOD;
        usleep(sleep_ms * 1000);
    }

    return 0;
}
