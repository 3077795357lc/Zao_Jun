/* NTP 对时：一个 UDP 请求 + 解出时间戳，不需要额外的库 */
#include "net/ntp.h"

#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* 阿里云 NTP，国内可达性较好；换成路由器或内网 NTP 服务器也行 */
#define NTP_SERVER "ntp.aliyun.com"
#define NTP_PORT   "123"

/* NTP 时间戳以 1900 年为起点，Unix 以 1970 年为起点，差这 70 年 */
#define NTP_UNIX_OFFSET 2208988800UL

/* 请求报文 48 字节；响应里偏移 40 的 8 字节是服务器"发送时间戳"，
 * 高 32 位是秒，低 32 位是小数部分（精度足够，这里直接丢弃小数） */
#define NTP_PKT_LEN      48
#define NTP_TX_TIME_OFS  40

static uint32_t ntp_be32_to_sec(const unsigned char *p)
{
    uint32_t ntp_sec = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                       ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
    return ntp_sec - NTP_UNIX_OFFSET;
}

int ntp_sync(int timeout_ms)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct timeval tv;
    unsigned char buf[NTP_PKT_LEN];
    int fd = -1;
    int ret = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        /* 板子环境简单，先用 IPv4 */
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(NTP_SERVER, NTP_PORT, &hints, &res) != 0) {
        printf("ntp: 解析 %s 失败，检查板子网络与 DNS\n", NTP_SERVER);
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        printf("ntp: socket 创建失败\n");
        goto out;
    }

    /* 接收超时：没联网时也要能返回，不能把启动流程挂住 */
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* 首字节 0x1B = LI 0、版本 3、模式 3(客户端)，其余 47 字节为 0 */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x1B;

    if (sendto(fd, buf, sizeof(buf), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("ntp: 请求发送失败\n");
        goto out;
    }

    if (recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL) < NTP_PKT_LEN) {
        printf("ntp: 没有收到响应（超时或网络不通）\n");
        goto out;
    }

    tv.tv_sec  = ntp_be32_to_sec(&buf[NTP_TX_TIME_OFS]);
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) < 0) {
        printf("ntp: 设置系统时间失败（需要 root 权限）\n");
        goto out;
    }

    printf("ntp: 对时成功\n");
    ret = 0;

out:
    if (fd >= 0) {
        close(fd);
    }
    freeaddrinfo(res);
    return ret;
}
