#include "dev_status.h"

//状态表：数组存放结构体,多少行看max
static DeviceID_State_t g_dev[DEVICE_MAX];

//互斥锁，管理状态表读取和存放的行为，避免竞态条件
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

//先初始化，初始化的设备先是关闭状态的
void dev_status_init(void){
    int i;
    for ( i = 0; i < DEVICE_MAX; i++)
    {
        g_dev[i].id = (DeviceID_t)i;
        g_dev[i].status = DEVICE_STATUS_OFF;
        g_dev[i].last_update = 0;
    }
}

//获取设备状态
DeviceStatus_t get_dev_status(DeviceID_t id){
    DeviceStatus_t s;

    //越界保护
    if (id < 0 || id > DEVICE_MAX){
        return DEVICE_STATUS_OFF;
        //printf
    }

    //读取status,返回状态值     
    pthread_mutex_lock(&g_lock);
    s = g_dev[id].status;
    pthread_mutex_unlock(&g_lock);    
    return s;
}

//更改状态表的某个设备的状态
void set_dev_status(DeviceID_t id){
    // 越界保护
    if (id < 0 || id >= DEVICE_MAX) {
        return;
    }

    // 加锁，切换设备状态，解锁
    pthread_mutex_lock(&g_lock);
    if (g_dev[id].status == DEVICE_STATUS_OFF) {
        g_dev[id].status = DEVICE_STATUS_ON;
    } else {
        g_dev[id].status = DEVICE_STATUS_OFF;
    }
    g_dev[id].last_update = time(NULL);
    pthread_mutex_unlock(&g_lock);
}



