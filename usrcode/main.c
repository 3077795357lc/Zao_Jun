/* 板级程序入口：只负责初始化与主循环，界面代码在 usrcode/ui/ 下 */
#include <stdio.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lvgl/drivers/display/lv_linux_fbdev.h"
#include "lvgl/drivers/indev/lv_evdev.h"

#include "ui/ui.h"

#include "hardware/env_status.h"
#include "hardware/gpio.h"
#include "data/dev_status.h"
#include "net/ntp.h"

int main(void)
{
    /* 开机对时：放在最前面，网络不通最多等 2 秒，此时屏幕还没点亮，
     * 用户察觉不到；等界面出来时顶栏时钟已经是准的 */
    ntp_sync(2000);

    /* 硬件与状态表初始化。
     * air_con_init()/led_init() 会把设备显式置为关闭，
     * 与 dev_status_init() 的初始状态(OFF)对齐 —— 这两处必须成对修改，
     * 否则状态表会与真实硬件相反。 */
    dht11_init();
    air_con_init();
    led_init();
    dev_status_init();

    // LVGL 环境初始化
    lv_init();

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

    // 创建界面（控件与定时器都在 ui 层内部管理）
    ui_init();

    // LVGL 引擎主循环：执行到期定时器、按需重绘屏幕
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY)
            sleep_ms = LV_DEF_REFR_PERIOD;
        usleep(sleep_ms * 1000);
    }

    return 0;
}
