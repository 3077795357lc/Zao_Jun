#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*
 * 该文件定义了全局的类型和枚举值
 * 包括设备ID、设备状态、传感器采集状态等
 * 还需要定义几把锁，用于保护共享资源
 * 
 */

 /*要实现的功能：
    1.一个LED灯，表示灯具（设备ID为DEVICE_LIGHT）
    1.1 信号来了，要能控制灯的开启和关闭
    1.2 灯的开关状态要能被记录下来，输出到其他设备上，显示在UI界面上（设备状态为DEVICE_STATUS_ON或DEVICE_STATU0S_OFF）

    一.对内具体如何实现这些功能？
    一.1首先理清我们要用哪些资源
    一.1.1一个线程没法做两件事情，不可能既随时等待着传入信号，又能控制灯的开启关闭。
    一.1.2一个线程处理LED的开关，一个线程等待别人传入信号（共享资源需复习）
    一.1.3
    （等待信号）线程A：时刻等待别人传入信号，如果有，就发送信号给线程B
    （控制设备）线程B：若收到线程A传入的信号，则改变灯具开关状态
    （保存状态发送UI）线程C：实时保存灯具开关状态，将状态信息实时发送给UI


    2.GUI功能
    2.1主界面，顶部显示时间日期，有按钮和图案，能显示数据
    2.2主界面分为两侧，一侧是按键，点击按键可以切换到其他界面。另一侧是传感器信息，显示今天的温度湿度，灯具和空调的开关状态。
    2.3左侧按键有“设置”、“日志管理”、“查看摄像头”、“定时任务”、每一个子页面都有“返回主菜单”
    
    启动程序：
    启动主线程，主线程启动线程ui_shuaxing；
    线程ui_shuaxing：UI线程，负责不断刷新渲染界面
    线程

 */




 // 设备ID定义
typedef enum
{
    DEVICE_TEMP_SENSOR = 0,//温湿度传感器
    DEVICE_AIRCONDITIONER,//空调
    DEVICE_LIGHT,//灯
    DEVICE_CAMERA,//相机
    DEVICE_AUDIO,//音频
    DEVICE_DOOR,//门    
}DeviceID_t;

typedef enum
{
    DEVICE_STATUS_ON = 0,//设备开启
    DEVICE_STATU0S_OFF,//设备关闭
}DeviceStatus_t;



#endif
