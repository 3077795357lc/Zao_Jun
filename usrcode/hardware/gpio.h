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

#define led_IO "/sys/class/gpio/gpio115/value"
#define air_IO_A "/sys/class/gpio/gpio116/value"
#define air_IO_B "/sys/class/gpio/gpio117/value"

void led_init(void);
void led_ctrl(void);


#endif