#include "gpio.h"


void led_init(void){
    int fd;
    /* gpio115 已被导出（value 文件已存在）时不再初始化，
       否则重复写 direction "out" 会把输出电平强制拉低，导致无法翻转 */
    if (access("/sys/class/gpio/gpio115/value", F_OK) == 0) {// 文件存在 → 说明 gpio115 已经被 export 过了
        return;
    }
    fd = open("/sys/class/gpio/export",O_WRONLY);
    if (fd == -1) {
    perror("open /sys/class/gpio/export failed:");
    return;
    }
    write(fd,"115",3);
    close(fd);

    fd = open("/sys/class/gpio/gpio115/direction",O_WRONLY);
    if (fd == -1){
    perror("open /sys/class/gpio/gpio115/direction failed:");
    return;
    } 
    write(fd,"out",3);
    close(fd);
}

void led_ctrl(void){
    int fd;
    char buf[2] = {0};
    fd = open("/sys/class/gpio/gpio115/value",O_RDWR);
    if (fd == -1){
    perror("open /sys/class/gpio/gpio115/value failed:");
    return;//
    }
    read(fd,buf,1);
    lseek(fd, 0, SEEK_SET);
    int result = atoi(buf);
    if ( result == 1){
        write(fd,"0",1);
    }
    else if (result == 0){
        write(fd,"1",1);
    }
    else{
        printf("buf为其他值\n");
        return;
    }
    close(fd);
}