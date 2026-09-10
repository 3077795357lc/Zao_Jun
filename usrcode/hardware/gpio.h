#ifndef GPIO_H
#define GPIO_H
#include "common_type.h"

static int led_fd;
static int airA_fd;
static int airB_fd;

#define LED_GPIO    116
#define AIR_INA_GPIO 117   // 电机INA控制脚
#define AIR_INB_GPIO 118   // 电机INB控制脚

void led_init(void);
void led_set(bool on);
void air_con_init(void);
void air_con_set(bool on);


#endif