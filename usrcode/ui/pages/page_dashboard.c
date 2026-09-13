/* 主界面：环境信息（温湿度）+ 设备控制（灯具、空调）
 *
 * 所有控件更新（定时器回调、开关事件）都在 LVGL 线程内执行，
 * 数据来源是 data/ 状态表与 hardware/ 驱动，本文件不直接碰寄存器。
 */
#include <stdbool.h>
#include <stdio.h>

#include "pages/page.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "data/dev_status.h"
#include "data/env_status.h"
#include "hardware/gpio.h"

/* ---------------- 控件句柄 ---------------- */
// 温湿度数值
static lv_obj_t *temp_label;
static lv_obj_t *humi_label;

// 设备开关与状态文字
static lv_obj_t *led_sw;
static lv_obj_t *air_sw;
static lv_obj_t *led_state_label;
static lv_obj_t *air_state_label;

/* ============ 定时器回调 ============ */

/* 设备状态同步：每 50ms 用状态表刷新开关位置和“开/关”文字 */
static void status_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    //从状态表取状态
    bool led_on = get_dev_status(DEVICE_LIGHT);
    bool air_on = get_dev_status(DEVICE_AIRCONDITIONER);

    //把状态表的情况显示到ui开关上
    if (led_on != lv_obj_has_state(led_sw, LV_STATE_CHECKED)) {
        if (led_on) lv_obj_add_state(led_sw, LV_STATE_CHECKED);
        else        lv_obj_remove_state(led_sw, LV_STATE_CHECKED);
    }
    if (air_on != lv_obj_has_state(air_sw, LV_STATE_CHECKED)) {
        if (air_on) lv_obj_add_state(air_sw, LV_STATE_CHECKED);
        else        lv_obj_remove_state(air_sw, LV_STATE_CHECKED);
    }

    if (led_on) {
        lv_label_set_text(led_state_label, "开");
    } else {
        lv_label_set_text(led_state_label, "关");
    }

    if (air_on) {
        lv_label_set_text(air_state_label, "开");
    } else {
        lv_label_set_text(air_state_label, "关");
    }
}

/* 环境信息刷新：数据由采集线程写入 data/env_status 状态表 */
static void env_timer_cb(lv_timer_t *timer)
{
    EnvStatus_t env;

    (void)timer;

    if (get_env_status(&env) == 0) {
        lv_label_set_text_fmt(temp_label, "%d℃", (unsigned char)env.temp);
        lv_label_set_text_fmt(humi_label, "%d%%", (unsigned char)env.humi);
    }
}

/* ============ 设备开关事件 ============ */

/* 灯具开关：把开关位置当作目标状态，“设定”硬件与状态表。 */
static void led_switch_cb(lv_event_t *e)
{
    //sw为从事件中取出来的被点击的开关switch
    lv_obj_t *sw = lv_event_get_target(e);
    //want_on是用户想要的结果，LV_STATE_CHECKED意为“被勾选/打开”
    //lv_obj_has_state判断sw是否为LV_STATE_CHECKED
    bool want_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    DeviceStatus_t status;

    if (want_on) {
        status = DEVICE_STATUS_ON;
    } else {
        status = DEVICE_STATUS_OFF;
    }

    led_set(want_on);
    set_dev_status(DEVICE_LIGHT, status);
}

/* 空调开关，同灯具 */
static void air_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool want_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    DeviceStatus_t status;

    if (want_on) {
        status = DEVICE_STATUS_ON;
    } else {
        status = DEVICE_STATUS_OFF;
    }

    air_con_set(want_on);
    set_dev_status(DEVICE_AIRCONDITIONER, status);
}

/* ============ 界面组装 ============ */

/* 卡片小标题 */
static void section_title_create(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_font(title, &heiti_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_left(title, 6, 0);
}

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

/* ============ 页面入口 ============ */

void page_dashboard_create(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_radius(card, 24, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 18, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0xD9B48F), 0);
    lv_obj_set_style_pad_all(card, 22, 0);
    lv_obj_set_scrollable(card, false);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 12, 0);

    /* ---- 环境信息 ---- */
    section_title_create(card, "环境信息");

    lv_obj_t *temp_row = row_create(card, ICON_TEMP, 0xE74C3C, "温度");
    temp_label = value_create(temp_row, "--℃");

    lv_obj_t *humi_row = row_create(card, ICON_HUMI, 0x3498DB, "湿度");
    humi_label = value_create(humi_row, "--%");

    /* ---- 设备控制 ---- */
    section_title_create(card, "设备控制");

    lv_obj_t *led_row = row_create(card, ICON_BULB, 0xF39C12, "灯具");
    spacer_create(led_row);
    led_sw = switch_create(led_row, led_switch_cb);
    led_state_label = state_create(led_row);

    lv_obj_t *air_row = row_create(card, ICON_SNOW, 0x3498DB, "空调");
    spacer_create(air_row);
    air_sw = switch_create(air_row, air_switch_cb);
    air_state_label = state_create(air_row);

    /* 定时器只在这里注册一次，页面切走再切回来不会重复创建 */
    lv_timer_create(status_timer_cb, 50, NULL);   /* 设备状态同步 */
    lv_timer_create(env_timer_cb, 2000, NULL);    /* 环境信息显示 */
}
