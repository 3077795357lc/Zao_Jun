#ifndef _GPIO_
#define _GPIO_

#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include "../../../lvgl/lvgl.h"

static int led_fd;
static int airA_fd;
static int airB_fd;

#define LED_GPIO    116
#define AIR_INA_GPIO 117   // 电机INA控制脚
#define AIR_INB_GPIO 118   // 电机INB控制脚

void led_init(void);
void led_ctrl(void);
void air_con_init(void);
void air_con_ctrl(void);


#endif