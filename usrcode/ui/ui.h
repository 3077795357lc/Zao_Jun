#ifndef UI_H
#define UI_H

/* UI 层对外接口
 *
 * 使用方式：硬件与 LVGL 初始化完成后调用 ui_init()，
 * 之后由主循环持续调用 lv_timer_handler() 驱动界面。
 *
 * 注意：LVGL 不是线程安全的，所有界面更新都跑在调用
 * lv_timer_handler() 的那个线程里，其他线程请勿直接操作控件。
 */
void ui_init(void);

#endif /* UI_H */
