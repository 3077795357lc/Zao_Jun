/* 占位页：设置 / 日志管理 / 查看摄像头 / 音频播放 / 定时任务
 *
 * 这些页面还没有具体功能，先统一渲染成“图标 + 标题 + 开发中提示”。
 * 要实现某个页面时，把对应的 page_xxx_create() 从这里挪到独立的
 * page_xxx.c 文件里即可，ui_main.c 的导航表不用改。
 */
#include "pages/page.h"
#include "ui_icons.h"
#include "ui_theme.h"

static void stub_create(lv_obj_t *parent, const char *icon, const char *title)
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
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 18, 0);

    lv_obj_t *icon_label = lv_label_create(card);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_font(icon_label, &icons_32, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(COLOR_ACCENT), 0);

    lv_obj_t *title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &heiti_32, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_TEXT), 0);

    lv_obj_t *hint_label = lv_label_create(card);
    lv_label_set_text(hint_label, "功能开发中");
    lv_obj_set_style_text_font(hint_label, &heiti_32, 0);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(COLOR_TEXT_DIM), 0);
}

void page_settings_create(lv_obj_t *parent)
{
    stub_create(parent, ICON_GEAR, "设置");
}

void page_log_create(lv_obj_t *parent)
{
    stub_create(parent, ICON_LOG, "日志管理");
}

void page_camera_create(lv_obj_t *parent)
{
    stub_create(parent, ICON_CAM, "查看摄像头");
}

void page_audio_create(lv_obj_t *parent)
{
    stub_create(parent, ICON_MUSIC, "音频播放");
}

void page_schedule_create(lv_obj_t *parent)
{
    stub_create(parent, ICON_CLOCK, "定时任务");
}
