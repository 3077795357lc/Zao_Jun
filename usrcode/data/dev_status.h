#ifndef DEV_STATE_H
#define DEV_STATE_H

#include "common_type.h"

/* 设备状态管理接口
 * 本模块在内存中维护一张设备状态表（数组），供各线程读写：
 *   - 控制线程：设备动作后调用 set_dev_status 更新状态
 *   - UI / MQTT 线程：调用 get_dev_status 读取状态用于显示与上报
 * 所有对状态表的访问都通过本模块接口进行（内部已加锁），
 * 不要在其他地方直接访问状态数组。
 */

//设备状态表初始化
void dev_status_init(void);

//获取某设备当前开关状态
DeviceStatus_t get_dev_status(DeviceID_t id);

//设置某设备的开关状态
void set_dev_status(DeviceID_t id, DeviceStatus_t status);

#endif