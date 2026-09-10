/* 界面外壳：顶栏时钟 + 左侧导航 + 右侧内容区
 *
 * 页面切换逻辑集中在这里：侧栏按钮 -> page_show(索引)。
 * 每个页面首次进入时才构建控件，之后切回只是显示/隐藏，
 * 所以页面内部的定时器只会注册一次。
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "ui.h"
#include "ui_icons.h"
#include "ui_theme.h"
#include "pages/page.h"

/* 侧栏导航表：顺序即显示顺序，索引 0 是默认页面 */
static const Page_t pages[] = {
    { ICON_HOME,  "主界面",     page_dashboard_create },
    { ICON_GEAR,  "设置",       page_settings_create },
    { ICON_LOG,   "日志管理",   page_log_create },
    { ICON_CAM,   "查看摄像头", page_camera_create },
    { ICON_MUSIC, "音频播放",   page_audio_create },
    { ICON_CLOCK, "定时任务",   page_schedule_create },
};

#define PAGE_COUNT ((int)(sizeof(pages) / sizeof(pages[0])))

static lv_obj_t *content;                /* 右侧内容区，各页面的挂载点 */
static lv_obj_t *nav_btns[PAGE_COUNT];   /* 侧栏按钮，用于高亮当前页 */
static lv_obj_t *nav_icons[PAGE_COUNT];  /* 侧栏按钮里的图标，需单独换色 */
static lv_obj_t *page_obj[PAGE_COUNT];   /* 各页面根容器，首次进入时创建 */

static lv_obj_t *time_label;
static lv_obj_t *date_label;

/* 侧栏文字配色的两种状态 */
#define NAV_BG_ON       COLOR_ACCENT
#define NAV_BG_OFF      COLOR_SIDEBAR
#define NAV_TEXT_ON     0xFFFFFF
#define NAV_TEXT_OFF    0x7A5230

/* ============ 顶栏时钟 ============ */

/* 每秒刷新一次系统时间 */
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

/* ============ 页面切换 ============ */

static void page_show(int idx)
{
    if (idx < 0 || idx >= PAGE_COUNT) {
        return;
    }

    /* 首次进入才构建页面内容 */
    if (page_obj[idx] == NULL) {
        lv_obj_t *root = lv_obj_create(content);
        lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(root, 0, 0);
        lv_obj_set_style_pad_all(root, 0, 0);
        lv_obj_set_scrollable(root, false);
        page_obj[idx] = root;

        pages[idx].create(root);
    }

    /* 只显示当前页，其余隐藏（控件保留，避免重建与重复注册定时器） */
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (page_obj[i] == NULL) {
            continue;
        }
        if (i == idx) lv_obj_set_hidden(page_obj[i], false);
        else          lv_obj_set_hidden(page_obj[i], true);
    }

    /* 高亮当前页对应的侧栏按钮 */
    for (int i = 0; i < PAGE_COUNT; i++) {
        bool on = (i == idx);
        lv_obj_set_style_bg_color(nav_btns[i],
                                  lv_color_hex(on ? NAV_BG_ON : NAV_BG_OFF), 0);
        lv_obj_set_style_text_color(nav_btns[i],
                                    lv_color_hex(on ? NAV_TEXT_ON : NAV_TEXT_OFF), 0);
        lv_obj_set_style_text_color(nav_icons[i],
                                    lv_color_hex(on ? NAV_TEXT_ON : COLOR_ACCENT), 0);
    }
}

static void nav_btn_cb(lv_event_t *e)
{
    page_show((int)(intptr_t)lv_event_get_user_data(e));
}

/* ============ 界面组装 ============ */

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

/* 左侧导航栏：各页面的入口，垂直对齐 */
static void sidebar_create(lv_obj_t *scr)
{
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, SIDEBAR_W, CONTENT_H);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, SIDEBAR_X, CONTENT_Y);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_scrollable(bar, false);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(bar, SIDEBAR_GAP, 0);

    for (int i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(bar);
        lv_obj_set_size(btn, LV_PCT(100), SIDEBAR_BTN_H);
        lv_obj_set_style_radius(btn, 18, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_left(btn, 16, 0);
        lv_obj_set_style_pad_right(btn, 8, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(btn, 12, 0);
        /* 文字颜色设在按钮上，由子标签继承，切换页面时整行一起变色 */
        lv_obj_set_style_text_color(btn, lv_color_hex(NAV_TEXT_OFF), 0);
        lv_obj_add_event_cb(btn, nav_btn_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, pages[i].icon);
        lv_obj_set_style_text_font(icon, &icons_32, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_ACCENT), 0);

        lv_obj_t *text = lv_label_create(btn);
        lv_label_set_text(text, pages[i].name);
        lv_obj_set_style_text_font(text, &heiti_32, 0);

        nav_btns[i] = btn;
        nav_icons[i] = icon;
    }
}

/* 右侧内容区：位置固定，各页面挂在这里 */
static void content_create(lv_obj_t *scr)
{
    content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_W - PANEL_X - PANEL_RIGHT, CONTENT_H);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, PANEL_X, CONTENT_Y);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_scrollable(content, false);
}

/* ============ 对外接口 ============ */

void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();   /* 当前活动屏幕，新控件挂在下面 */
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_scrollable(scr, false);

    header_create(scr);
    content_create(scr);
    sidebar_create(scr);

    page_show(0);   /* 默认进入主界面 */

    lv_timer_create(clock_timer_cb, 1000, NULL);  /* 时间日期 */
}
