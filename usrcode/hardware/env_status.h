#ifndef ENV_STATUS_H
#define ENV_STATUS_H
#include "common_type.h"

#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* 温湿度数据：与驱动输出的原始字节一致 */
typedef struct env_status
{
    char temp;   // 温度
    char humi;   // 湿度
} EnvStatus_t;

void dht11_init(void);

/* 单次读取温湿度，成功返回0并把数据填入 env，失败返回-1 */
int dht11_read(EnvStatus_t *env);


#endif