/* 板级程序入口：只负责初始化与主循环，界面代码在 usrcode/ui/ 下 */
#include <stdio.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lvgl/drivers/display/lv_linux_fbdev.h"
#include "lvgl/drivers/indev/lv_evdev.h"

#include "ui/ui.h"

#include "hardware/gpio.h"
#include "data/dev_status.h"
#include "data/env_status.h"
#include "net/ntp.h"
#include "net/mqtt_client.h"

/* 温湿度采集线程。
 * 读到值后写入 data/env_status 状态表，UI 与 MQTT 再从状态表取数*/
static void *env_sampler_thread(void *arg)
{
    (void)arg;
    EnvStatus_t env;

    while (1) {
        if (dht11_read(&env) == 0) {
            set_env_status(&env);
        }
        sleep(2);
    }
    return NULL;
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* 开机对时：网络不通最多等 2 秒，*/
    ntp_sync(2000);

    // 硬件初始化。
    dht11_init();
    air_con_init();
    led_init();

    //状态表初始化
    dev_status_init();
    env_status_init();

    /* 起温湿度采集线程：负责读硬件并写状态表，其他线程只读状态表 */
    pthread_t env_tid;
    if (pthread_create(&env_tid, NULL, env_sampler_thread, NULL) != 0) {
        perror("pthread_create env_sampler failed");
    }

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
