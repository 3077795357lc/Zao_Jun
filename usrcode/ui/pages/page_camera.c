/*
 * 摄像头页面：左侧三个操作按钮，右侧实时监控画面。
 *
 * 分工：
 *   - hardware/camera_monitor.c 负责 V4L2 采集与格式转换（后台线程）；
 *   - 本文件只做两件事：按钮触发的启动/关闭，以及用定时器把最新一帧刷到 image 控件。
 *
 * 画面位置沿用 V4L2.c 的算法：(LCD 宽 - 帧宽) / 2 + 125，纵坐标 110。
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pages/page.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "hardware/camera_monitor.h"

/* 页面根容器由 ui_main.c 挂在内容区里，高度只有 456px，
 * 装不下 480px 高的监控画面。这里把根容器改挂到屏幕，并铺一块足够大的区域，
 * 这样画面可以完整显示，同时不影响其他页面继续使用内容区。 */
#define CAMERA_PAGE_LEFT      28
#define CAMERA_PAGE_TOP       100
#define CAMERA_PAGE_WIDTH     (SCREEN_W - CAMERA_PAGE_LEFT - 28)
#define CAMERA_PAGE_HEIGHT    (SCREEN_H - CAMERA_PAGE_TOP)

/* 与 V4L2.c 一致的画面位置（绝对屏幕坐标） */
#define CAMERA_PREVIEW_X      (((SCREEN_W - CAMERA_FRAME_WIDTH) / 2) + 125)
#define CAMERA_PREVIEW_Y      110

/* 换算成相对页面根容器的坐标 */
#define CAMERA_PREVIEW_OFFSET_X  (CAMERA_PREVIEW_X - CAMERA_PAGE_LEFT)
#define CAMERA_PREVIEW_OFFSET_Y  (CAMERA_PREVIEW_Y - CAMERA_PAGE_TOP)

#define CAMERA_BUTTON_WIDTH        250
#define CAMERA_BUTTON_HEIGHT       62
#define CAMERA_BUTTON_GAP          20
#define CAMERA_BUTTON_TOP          10
#define CAMERA_PREVIEW_REFRESH_MS  33

static lv_obj_t *preview_image;
static lv_obj_t *preview_hint_label;
static lv_obj_t *monitor_state_label;

/* 当前显示的是哪一块转换缓冲，用于判断是否需要重新设置图片源 */
static const uint32_t *preview_pixels;

/* XRGB8888 图像描述：data 指向 camera_monitor 的双缓冲，随最新帧切换 */
static lv_image_dsc_t preview_image_dsc = {
    .header = {
        .magic  = LV_IMAGE_HEADER_MAGIC,
        .cf     = LV_COLOR_FORMAT_XRGB8888,
        .w      = CAMERA_FRAME_WIDTH,
        .h      = CAMERA_FRAME_HEIGHT,
        .stride = CAMERA_FRAME_WIDTH * 4,
    },
    .data_size = CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * 4,
    .data = NULL,
};

static void camera_state_set_text(const char *text)
{
    lv_label_set_text(monitor_state_label, text);
}

static void camera_preview_set_hint(const char *text)
{
    lv_label_set_text(preview_hint_label, text);
    lv_obj_set_hidden(preview_hint_label, false);
}

/* ============================ 按钮回调 ============================ */

static void camera_start_clicked(lv_event_t *event)
{
    (void)event;

    if (camera_monitor_is_running()) {
        camera_state_set_text("状态：监控中");
        return;
    }

    if (camera_monitor_start() == 0) {
        camera_state_set_text("状态：监控中");
        camera_preview_set_hint("摄像头连接中...");
    } else {
        camera_state_set_text("状态：摄像头未连接");
        camera_preview_set_hint("摄像头未连接");
    }
}

static void camera_stop_clicked(lv_event_t *event)
{
    (void)event;

    camera_monitor_stop();

    preview_pixels = NULL;
    lv_obj_set_hidden(preview_image, true);
    camera_preview_set_hint("监控已关闭");
    camera_state_set_text("状态：已关闭");
}

static void camera_take_photo_clicked(lv_event_t *event)
{
    (void)event;

    /* 拍照功能暂未实现：保存位置确定后再补 */
    camera_state_set_text("状态：拍照功能待实现");
}

/* ============================ 画面刷新 ============================ */

/* LVGL 主线程里的定时器：有新帧就切换图片源并重绘。
 * 采集和转换都在后台线程完成，这里只做指针切换与重绘请求。 */
static void preview_refresh_timer_cb(lv_timer_t *timer)
{
    const uint32_t *new_pixels = NULL;

    (void)timer;

    if (!camera_monitor_take_frame(&new_pixels)) {
        return;
    }

    if (new_pixels != preview_pixels) {
        preview_pixels = new_pixels;
        preview_image_dsc.data = (const uint8_t *)new_pixels;
        lv_image_set_src(preview_image, &preview_image_dsc);
        lv_obj_set_hidden(preview_image, false);
        lv_obj_set_hidden(preview_hint_label, true);
    }

    lv_obj_invalidate(preview_image);
}

/* ============================ 控件构建 ============================ */

static lv_obj_t *camera_button_create(lv_obj_t *parent, const char *text,
                                      int top_offset, lv_event_cb_t callback)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label;

    lv_obj_set_size(button, CAMERA_BUTTON_WIDTH, CAMERA_BUTTON_HEIGHT);
    lv_obj_set_pos(button, 0, top_offset);
    lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_SIDEBAR), 0);
    lv_obj_set_style_radius(button, 18, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &heiti_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_center(label);

    return button;
}

static void camera_preview_create(lv_obj_t *parent)
{
    lv_obj_t *preview_frame = lv_obj_create(parent);

    lv_obj_set_pos(preview_frame, CAMERA_PREVIEW_OFFSET_X, CAMERA_PREVIEW_OFFSET_Y);
    lv_obj_set_size(preview_frame, CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);
    lv_obj_set_style_bg_color(preview_frame, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(preview_frame, 0, 0);
    lv_obj_set_style_border_width(preview_frame, 0, 0);
    lv_obj_set_style_pad_all(preview_frame, 0, 0);
    lv_obj_set_scrollable(preview_frame, false);

    preview_hint_label = lv_label_create(preview_frame);
    lv_label_set_text(preview_hint_label, "摄像头未连接");
    lv_obj_set_style_text_font(preview_hint_label, &heiti_32, 0);
    lv_obj_set_style_text_color(preview_hint_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(preview_hint_label);

    preview_image = lv_image_create(preview_frame);
    lv_obj_set_pos(preview_image, 0, 0);
    lv_obj_set_hidden(preview_image, true);
}

void page_camera_create(lv_obj_t *parent)
{
    int button_top;

    /* 脱离内容区裁剪：挂到屏幕并铺成足够大的区域 */
    lv_obj_set_parent(parent, lv_screen_active());
    lv_obj_set_pos(parent, CAMERA_PAGE_LEFT, CAMERA_PAGE_TOP);
    lv_obj_set_size(parent, CAMERA_PAGE_WIDTH, CAMERA_PAGE_HEIGHT);
    lv_obj_set_clickable(parent, false);
    lv_obj_set_scrollable(parent, false);

    /* 左侧三个操作按钮，纵向排列 */
    button_top = CAMERA_BUTTON_TOP;
    camera_button_create(parent, "开启监控", button_top, camera_start_clicked);

    button_top += CAMERA_BUTTON_HEIGHT + CAMERA_BUTTON_GAP;
    camera_button_create(parent, "关闭监控", button_top, camera_stop_clicked);

    button_top += CAMERA_BUTTON_HEIGHT + CAMERA_BUTTON_GAP;
    camera_button_create(parent, "拍照", button_top, camera_take_photo_clicked);

    /* 按钮下方显示当前状态 */
    button_top += CAMERA_BUTTON_HEIGHT + CAMERA_BUTTON_GAP;
    monitor_state_label = lv_label_create(parent);
    lv_label_set_text(monitor_state_label, "状态：未开启");
    lv_obj_set_pos(monitor_state_label, 0, button_top);
    lv_obj_set_width(monitor_state_label, CAMERA_BUTTON_WIDTH);
    lv_obj_set_style_text_font(monitor_state_label, &heiti_32, 0);
    lv_obj_set_style_text_color(monitor_state_label, lv_color_hex(COLOR_TEXT_DIM), 0);

    /* 右侧监控画面 */
    camera_preview_create(parent);

    /* 只在页面可见时按帧率取图；页面隐藏时定时器仍会运行，但不会有新帧来源 */
    lv_timer_create(preview_refresh_timer_cb, CAMERA_PREVIEW_REFRESH_MS, NULL);
}
