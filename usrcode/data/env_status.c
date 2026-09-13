#include "env_status.h"

//状态表：一份温湿度数值 + 是否已采到有效数据
static EnvStatus_t     g_env;
static int             g_valid = 0;

//互斥锁：采集线程写、UI/MQTT 线程读，避免竞态
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

void env_status_init(void){
    g_env.temp = 0;
    g_env.humi = 0;
    g_valid = 0;
}

//将状态表的数据g_env传给ui显示的数据env
int get_env_status(EnvStatus_t *env){
    int ret = -1;

    if (env == NULL){
        return -1;
    }

    pthread_mutex_lock(&g_lock);
    if (g_valid){
        *env = g_env;
        ret = 0;
    }
    pthread_mutex_unlock(&g_lock);
    return ret;
}

//将从dht11读取的数据env给状态表g_env
void set_env_status(const EnvStatus_t *env){
    if (env == NULL){
        return;
    }

    //g_env为状态表，env为从dht11读出来的数据
    pthread_mutex_lock(&g_lock);
    g_env = *env;
    g_valid = 1;
    pthread_mutex_unlock(&g_lock);
}
