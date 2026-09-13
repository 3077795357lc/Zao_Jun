#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

/* 设备端 MQTT 客户端对外接口
 * 连接、订阅、断线重连全部在库线程里自动完成，
 * 启动后立即返回，不会卡住 LVGL 主线程。
 */

/* 启动客户端：连接 Broker、订阅控制主题、开始周期上报。*/
int  mqtt_client_start(void);

/* 停止客户端：停止上报线程、断开连接、释放资源。 */
void mqtt_client_stop(void);

#endif
