#ifndef COMMON_TYPE_H
#define COMMON_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// #include "../../../lvgl/lvgl.h"
 // 设备ID定义
typedef enum
{
    DEVICE_TEMP_SENSOR = 0,//温湿度传感器
    DEVICE_AIRCONDITIONER,//空调
    DEVICE_LIGHT,//灯
    DEVICE_MAX,
    //后续可以再添加摄像头、音频设备
}DeviceID_t;

typedef enum
{
    DEVICE_STATUS_ON = 0,//设备开启
    DEVICE_STATUS_OFF,//设备关闭
}DeviceStatus_t;

typedef struct{
    DeviceID_t id;
    DeviceStatus_t status;
    uint64_t last_update;// 上次状态更新时间（CLOCK_MONOTONIC，秒）
}DeviceID_State_t;


#endif
