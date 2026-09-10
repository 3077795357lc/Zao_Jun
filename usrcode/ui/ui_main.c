/* 主界面：顶栏时间日期 + 左侧导航栏 + 右侧温湿度与设备控制卡片
 *
 * 所有控件更新（定时器回调、开关事件）都在 LVGL 线程内执行，
 * 数据来源是 data/ 状态表与 hardware/ 驱动，本文件不直接碰寄存器。
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ui.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "data/dev_status.h"
#include "hardware/env_status.h"
#include "hardware/gpio.h"

/* ---------------- 控件句柄 ---------------- */
// 顶栏：时间日期
static lv_obj_t *time_label;
static lv_obj_t *date_label;

// 温湿度数值
static lv_obj_t *temp_label;
static lv_obj_t *humi_label;

// 设备开关与状态文字
static lv_obj_t *led_sw;
static lv_obj_t *air_sw;
static lv_obj_t *led_state_label;
static lv_obj_t *air_state_label;

//环境状态
static EnvStatus_t env;

/* ============ 定时器回调 ============ */

/* 顶栏时钟：每秒刷新一次系统时间 */
static void clock_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    static const char *week[] = {"日", "一", "二", "三", "四", "五", "六"};
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[64];

    snprintf(buf, sizeof(buf), "%04d年%02d月%02d日 星期%s",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, week[t->tm_wday]);
    lv_label_set_text(date_label, buf);

    strftime(buf, sizeof(buf), "%H:%M:%S", t);
    lv_label_set_text(time_label, buf);
}

/* 设备状态同步：每 50ms 用状态表刷新开关位置和“开/关”文字 */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    bool led_on = (DEVICE_STATUS_ON == get_dev_status(DEVICE_LIGHT));
    bool air_on = (DEVICE_STATUS_ON == get_dev_status(DEVICE_AIRCONDITIONER));

    if (led_on != lv_obj_has_state(led_sw, LV_STATE_CHECKED)) {
        if (led_on) lv_obj_add_state(led_sw, LV_STATE_CHECKED);
        else        lv_obj_remove_state(led_sw, LV_STATE_CHECKED);
    }
    if (air_on != lv_obj_has_state(air_sw, LV_STATE_CHECKED)) {
        if (air_on) lv_obj_add_state(air_sw, LV_STATE_CHECKED);
        else        lv_obj_remove_state(air_sw, LV_STATE_CHECKED);
    }

    lv_label_set_text(led_state_label, led_on ? "开" : "关");
    lv_label_set_text(air_state_label, air_on ? "开" : "关");
}

/* 温湿度采集：DHT11 两次读取需间隔 >=1s，这里每 2s 读一次 */
static void dht11_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (dht11_read(&env) == 0) {
        lv_label_set_text_fmt(temp_label, "%d℃", (unsigned char)env.temp);
        lv_label_set_text_fmt(humi_label, "%d%%", (unsigned char)env.humi);
    }
}

/* ============ 设备开关事件 ============ */

/* 灯具开关：只在期望状态与状态表不一致时才翻转，避免重复触发硬件 */
static void led_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool want_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (want_on != (DEVICE_STATUS_ON == get_dev_status(DEVICE_LIGHT))) {
        led_ctrl();
        set_dev_status(DEVICE_LIGHT);
    }
}

/* 空调开关 */
static void air_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool want_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (want_on != (DEVICE_STATUS_ON == get_dev_status(DEVICE_AIRCONDITIONER))) {
        air_con_ctrl();
        set_dev_status(DEVICE_AIRCONDITIONER);
    }
}

/* ============ UI 组装 ============ */

/* 卡片内的一行：[图标][名称] ……，返回该行供调用者继续追加内容 */
static lv_obj_t *row_create(lv_obj_t *parent, const char *icon,
                            uint32_t icon_color, const char *name)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 64);
    lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_ROW), 0);
    lv_obj_set_style_radius(row, 14, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 18, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 14, 0);

    lv_obj_t *icon_label = lv_label_create(row);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_font(icon_label, &icons_32, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(icon_color), 0);
    lv_obj_set_width(icon_label, 40);
    lv_obj_set_style_text_align(icon_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, &heiti_32, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_width(name_label, 120);

    return row;
}

/* 弹性占位：把后面的控件推到行尾 */
static void spacer_create(lv_obj_t *row)
{
    lv_obj_t *spacer = lv_obj_create(row);
    lv_obj_set_size(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_pad_all(spacer, 0, 0);
}

/* 卡片小标题 */
static void section_title_create(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_font(title, &heiti_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_left(title, 6, 0);
}

/* 数值文字（温度 / 湿度） */
static lv_obj_t *value_create(lv_obj_t *row, const char *text)
{
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &heiti_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
    return label;
}

/* 滑动开关：槽 + 球；开启时槽变绿 */
static lv_obj_t *switch_create(lv_obj_t *row, lv_event_cb_t cb)
{
    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 76, 40);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COLOR_SW_OFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COLOR_SW_ON),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(sw, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, NULL);
    return sw;
}

/* 设备状态文字：开 / 关 */
static lv_obj_t *state_create(lv_obj_t *row)
{
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, "关");
    lv_obj_set_style_text_font(label, &heiti_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT_DIM), 0);
    lv_obj_set_width(label, 48);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
    return label;
}

/* 顶栏：时间（大字）+ 日期，居中显示 */
static void header_create(lv_obj_t *scr)
{
    lv_obj_t *box = lv_obj_create(scr);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_scrollable(box, false);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(box);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x6B4E31), 0);

    date_label = lv_label_create(box);
    lv_label_set_text(date_label, "0000年00月00日 星期0");
    lv_obj_set_style_text_font(date_label, &heiti_32, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(COLOR_TEXT_DIM), 0);
}

/* 左侧导航栏：5 个功能入口（跳转暂未实现），垂直对齐 */
static void sidebar_create(lv_obj_t *scr)
{
    static const char *names[] = {"设置", "日志管理", "查看摄像头", "音频播放", "定时任务"};
    static const char *icons[] = {ICON_GEAR, ICON_LOG, ICON_CAM, ICON_MUSIC, ICON_CLOCK};

    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, SIDEBAR_W, CONTENT_H);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, SIDEBAR_X, CONTENT_Y);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_scrollable(bar, false);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(bar, 16, 0);

    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(bar);
        lv_obj_set_size(btn, LV_PCT(100), 68);
        lv_obj_set_style_radius(btn, 18, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_SIDEBAR), 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_left(btn, 16, 0);
        lv_obj_set_style_pad_right(btn, 8, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(btn, 12, 0);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, icons[i]);
        lv_obj_set_style_text_font(icon, &icons_32, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_ACCENT), 0);

        lv_obj_t *text = lv_label_create(btn);
        lv_label_set_text(text, names[i]);
        lv_obj_set_style_text_font(text, &heiti_32, 0);
        lv_obj_set_style_text_color(text, lv_color_hex(0x7A5230), 0);
    }
}

/* 右侧主卡片：环境信息 + 设备控制 */
static void panel_create(lv_obj_t *scr)
{
    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_size(panel, SCREEN_W - PANEL_X - PANEL_RIGHT, CONTENT_H);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, PANEL_X, CONTENT_Y);
    lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_radius(panel, 24, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_shadow_width(panel, 18, 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(panel, lv_color_hex(0xD9B48F), 0);
    lv_obj_set_style_pad_all(panel, 22, 0);
    lv_obj_set_scrollable(panel, false);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 12, 0);

    /* ---- 环境信息 ---- */
    section_title_create(panel, "环境信息");

    lv_obj_t *temp_row = row_create(panel, ICON_TEMP, 0xE74C3C, "温度");
    temp_label = value_create(temp_row, "--℃");

    lv_obj_t *humi_row = row_create(panel, ICON_HUMI, 0x3498DB, "湿度");
    humi_label = value_create(humi_row, "--%");

    /* ---- 设备控制 ---- */
    section_title_create(panel, "设备控制");

    lv_obj_t *led_row = row_create(panel, ICON_BULB, 0xF39C12, "灯具");
    spacer_create(led_row);
    led_sw = switch_create(led_row, led_switch_cb);
    led_state_label = state_create(led_row);

    lv_obj_t *air_row = row_create(panel, ICON_SNOW, 0x3498DB, "空调");
    spacer_create(air_row);
    air_sw = switch_create(air_row, air_switch_cb);
    air_state_label = state_create(air_row);
}

/* ============ 对外接口 ============ */

void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();   /* 当前活动屏幕，新控件挂在下面 */
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_scrollable(scr, false);

    header_create(scr);
    sidebar_create(scr);
    panel_create(scr);

    lv_timer_create(ui_timer_cb, 50, NULL);       /* 设备状态同步 */
    lv_timer_create(dht11_timer_cb, 2000, NULL);  /* 温湿度采集 */
    lv_timer_create(clock_timer_cb, 1000, NULL);  /* 时间日期 */
}
