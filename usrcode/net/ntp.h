#ifndef NTP_H
#define NTP_H

/* 网络时间同步
 *
 * 向 NTP 服务器发一次请求，成功则用返回的时间设置系统时间。
 * 系统时间准了，顶栏时钟自然就准了，界面侧不用做任何改动。
 *
 * timeout_ms 是等待响应的上限。板子没联网时必须能超时返回，
 * 否则会把启动流程卡死在这里。
 *
 * 成功返回 0，失败返回 -1 并打印原因。
 */
int ntp_sync(int timeout_ms);

#endif /* NTP_H */
