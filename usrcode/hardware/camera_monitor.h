#ifndef CAMERA_MONITOR_H
#define CAMERA_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

/* 摄像头采集参数：与 V4L2.c 保持一致 */
#define CAMERA_DEVICE_PATH   "/dev/video1"
#define CAMERA_FRAME_WIDTH   640
#define CAMERA_FRAME_HEIGHT  480

/* 启动监控：打开设备、配置 YUYV 格式、申请并映射帧缓冲、开启视频流，
 * 同时创建一个后台采集线程。成功返回 0，失败返回 -1。 */
int camera_monitor_start(void);

/* 关闭监控：通知采集线程退出并等待其结束，然后关流、解除映射、关闭设备。
 * 未启动时调用也安全，可重复调用。 */
void camera_monitor_stop(void);

/* 监控是否已开启（采集线程已创建且尚未停止）。 */
bool camera_monitor_is_running(void);

/* 取最新一帧（XRGB8888）。有新帧时返回 true，并通过 frame_pixels 输出像素首地址；
 * 没有新帧时返回 false，且不改动 frame_pixels。
 * 返回的地址在下一次取帧之前保持有效。 */
bool camera_monitor_take_frame(const uint32_t **frame_pixels);

#endif /* CAMERA_MONITOR_H */
