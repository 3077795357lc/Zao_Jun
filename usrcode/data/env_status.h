#ifndef ENV_STATE_H
#define ENV_STATE_H

#include "common_type.h"

/* 环境状态表：存放温湿度数值，将硬件和软件解耦。*/

//环境状态表初始化：初始为“无有效数据”
void env_status_init(void);

//读取最近一次有效读数；尚无有效数据时返回 -1，成功返回 0
int get_env_status(EnvStatus_t *env);

//写入一次有效读数
void set_env_status(const EnvStatus_t *env);

#endif
