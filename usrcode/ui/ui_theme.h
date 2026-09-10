#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl/lvgl.h"

/* ---------------- 配色（温馨暖色调） ---------------- */
#define COLOR_BG       0xFDF6EC   /* 屏幕底色：暖米白 */
#define COLOR_CARD     0xFFFFFF   /* 卡片：白 */
#define COLOR_ROW      0xFFF9F0   /* 行底色：浅奶油 */
#define COLOR_TEXT     0x5B4636   /* 主文字：暖棕 */
#define COLOR_TEXT_DIM 0xA67C52   /* 次要文字：浅棕 */
#define COLOR_SIDEBAR  0xF7E4C7   /* 侧栏按钮：暖沙 */
#define COLOR_ACCENT   0xE08C2E   /* 强调色：暖橙 */
#define COLOR_SW_OFF   0xCDBBA6   /* 开关槽（关）：暖灰 */
#define COLOR_SW_ON    0x4CAF50   /* 开关槽（开）：绿色 */

/* ---------------- 界面尺寸（屏幕 1024x600） ---------------- */
#define SCREEN_W       1024
#define SCREEN_H       600
#define SIDEBAR_W      250        /* 左侧导航栏宽度 */
#define SIDEBAR_X      28
#define SIDEBAR_BTN_H  62         /* 侧栏按钮高度（6 个入口要放得下） */
#define SIDEBAR_GAP    16         /* 侧栏按钮间距 */
#define PANEL_X        300        /* 右侧卡片起点 */
#define PANEL_RIGHT    28         /* 右侧卡片右边距 */
#define CONTENT_Y      122        /* 内容区顶部（让开顶栏时钟） */
#define CONTENT_H      456        /* 内容区高度 */

/* ---------------- 字体（字形文件在 usrcode/assets/fonts/） ---------------- */
LV_FONT_DECLARE(heiti_32);   /* 中文黑体，含 ℃ */
LV_FONT_DECLARE(icons_32);   /* 图标字体（FontAwesome 6 Free，CC BY 4.0） */

#endif /* UI_THEME_H */
