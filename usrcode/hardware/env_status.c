#include "env_status.h"

static int dht_fd;

//打开/dev/mydht11
void dht11_init(void){
    //open
    dht_fd = open("/dev/mydht11", O_RDWR | O_NONBLOCK);
    if (dht_fd == -1)
    {
        perror("open dht11 failed");
        return;
    }
}

//单次读取数据，对穿入的两个参数humi和temp返回温湿度值
int dht11_read(char *humi,char *temp){
    if (dht_fd == -1){
        perror("read dht_fd failed");
        return -1;
    }
    
    char buf[2];
    if (read(dht_fd,buf,2) == 2){
        *humi = buf[0];
        *temp = buf[1];
    }
    else{
        perror("read dht_fd failed");
        return -1;
    }
    return 0;
}