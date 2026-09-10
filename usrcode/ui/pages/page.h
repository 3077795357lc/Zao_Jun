#ifndef PAGE_H
#define PAGE_H

#include "lvgl/lvgl.h"

/* 页面接口
 *
 * 每个页面实现一个 create()，在传入的容器里构建自己的控件。
 * 页面的根容器、显示/隐藏、切换时机都由 ui_main.c 统一管理，
 * 页面自身不需要关心导航逻辑。
 *
 * create() 只在页面首次进入时调用一次，之后切回来只是重新显示，
 * 所以可以在里面放心地 lv_timer_create()，不会重复注册。
 */
typedef struct
{
    const char *icon;                  /* 侧栏图标字形，见 ui_icons.h */
    const char *name;                  /* 侧栏文字 */
    void (*create)(lv_obj_t *parent);  /* 首次进入时构建页面内容 */
} Page_t;

/* 各页面实现，顺序与侧栏导航一致 */
void page_dashboard_create(lv_obj_t *parent);   /* 主界面 */
void page_settings_create(lv_obj_t *parent);    /* 设置（占位） */
void page_log_create(lv_obj_t *parent);         /* 日志管理（占位） */
void page_camera_create(lv_obj_t *parent);      /* 查看摄像头（占位） */
void page_audio_create(lv_obj_t *parent);       /* 音频播放（占位） */
void page_schedule_create(lv_obj_t *parent);    /* 定时任务（占位） */

#endif /* PAGE_H */
