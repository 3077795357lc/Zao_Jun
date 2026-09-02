#include "gpio.h"

/* 将一个 gpio 引脚导出并配置为输出方向。
   若该引脚已导出且方向已是 out 则直接返回，
   避免重复写 direction 把输出电平强制拉低。*/
static void gpio_export_output(int gpio_num){
    char num[8];
    char path[64];
    int fd;

    //已导出且方向正确,跳出初始化
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);
    if(access(path, F_OK) == 0){
        char cur[8] = {0};
        fd = open(path, O_RDONLY);
        if(fd != -1){
            read(fd, cur, sizeof(cur)-1);
            close(fd);
        }
        if(strncmp(cur, "out", 3) == 0){
            return;
        }
        //方向不是 out（残留的 in），继续往下把它修正为 out
        fd = open(path, O_WRONLY);
        if(fd == -1){
            perror("open gpio direction failed:");
            return;
        }
        if(write(fd, "out", 3) == -1){
            perror("write gpio direction failed:");
            close(fd);
            return;
        }
        close(fd);
        return;
    }

    //未导出，则先导出引脚
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd == -1){
        perror("open /sys/class/gpio/export failed:");
        return;
    }
    snprintf(num, sizeof(num), "%d", gpio_num);

    if(write(fd, num, strlen(num)) == -1){
        perror("write gpio export failed:");
        close(fd);
        return;
    }
    close(fd);

    //再配置输出方向
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_num);
    fd = open(path, O_WRONLY);
    if(fd == -1){
        perror("open gpio direction failed:");
        return;
    }
    if(write(fd, "out", 3) == -1){//写方向失败时关闭fd后返回
        perror("write gpio direction failed:");
        close(fd);
        return;
    }
    close(fd);
}

void led_init(void){
    gpio_export_output(LED_GPIO);
}

void led_ctrl(void){
    int fd;
    char buf[2] = {0};
    char path[64];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", LED_GPIO);
    fd = open(path, O_RDWR);
    if(fd == -1){
        perror("open led gpio value failed:");
        return;
    }
    
    //读value
    if(read(fd, buf, 1) == -1){
        perror("read led gpio value failed:");
        close(fd);
        return;
    }
    lseek(fd, 0, SEEK_SET);

    //控制开关-原来是关，则打开；原来是开，则关闭；
    if(buf[0] == '1'){
        if(write(fd, "0", 1) == -1){
            perror("write led gpio value failed:");
            close(fd);
            return;
        }
    }
    else if(buf[0] == '0'){
        if(write(fd, "1", 1) == -1){
            perror("write led gpio value failed:");
            close(fd);
            return;
        }
    }
    else{//非法值则关闭 fd 后返回
        printf("buf为其他值:%s\n", buf);
        close(fd);
        return;
    }
    close(fd);
}

void air_con_init(void){
    //分别导出 INA/INB 引脚并配置为输出
    gpio_export_output(AIR_INA_GPIO);
    gpio_export_output(AIR_INB_GPIO);
}

void air_con_ctrl(void){
    int fd_a;
    int fd_b;
    char buf_a[2] = {0};
    char buf_b[2] = {0};
    char write_a;
    char write_b;
    char path[64];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", AIR_INA_GPIO);
    fd_a = open(path, O_RDWR);
    if(fd_a == -1){
        perror("open INA gpio value failed:");
        return;
    }
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", AIR_INB_GPIO);
    fd_b = open(path, O_RDWR);
    if(fd_b == -1){
        perror("open INB gpio value failed:");
        close(fd_a);//释放fd_a，避免泄漏
        return;
    }

    //读取两个引脚当前电平，任一失败都同时关闭两个fd
    if(read(fd_a, buf_a, 1) == -1){
        perror("read INA gpio value failed:");
        close(fd_a);
        close(fd_b);
        return;
    }
    lseek(fd_a, 0, SEEK_SET);
    if(read(fd_b, buf_b, 1) == -1){
        perror("read INB gpio value failed:");
        close(fd_a);
        close(fd_b);
        return;
    }
    lseek(fd_b, 0, SEEK_SET);

    int result_a = atoi(buf_a);
    int result_b = atoi(buf_b);

    if( (result_a == 0 && result_b == 1) ||
        (result_a == 1 && result_b == 0)  ){
        //电机当前在转动，则停止（IA=0, IB=0）
        write_a = '0';
        write_b = '0';
    }
    else{
        //电机当前停止/刹车，则正转（IA=1, IB=0）
        write_a = '1';
        write_b = '0';
    }

    //写入并关闭INA和INB
    if( (write(fd_a, &write_a, 1) == -1) || (write(fd_b, &write_b, 1) == -1) ){
        perror("write gpio value failed:");
        close(fd_a);
        close(fd_b);
        return;
    }

    close(fd_a);
    close(fd_b);
}
