#include "env_status.h"

static int dht_fd;

//打开/dev/mydht11
void dht11_init(void){
    //open
    dht_fd = open("/dev/mydht11_poll", O_RDWR);
    if (dht_fd == -1)
    {
        perror("open dht11 failed");
        return;
    }
    printf("fd = %d\n",dht_fd);
}

//单次读取数据，把温湿度填入 env 结构体
int dht11_read(EnvStatus_t *env){
    if (dht_fd == -1){
        perror("read dht_fd failed");
        return -1;
    }
    if (env == NULL){
        return -1;
    }
    
    char buf[2];
    if (read(dht_fd,buf,2) == 2){
        env->humi = buf[0];
        env->temp = buf[1];
    }
    else{
        perror("read dht_fd failed");
        return -1;
    }
    return 0;
}

