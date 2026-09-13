#include "gpio.h"
#include <errno.h>

static int dht_fd = -1;
/* 将一个 gpio 引脚导出并配置为输出方向。
   若该引脚已导出且方向已是 out 则直接返回*/
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

/* 把引脚电平写为指定状态（true=高电平，false=低电平） */
static void gpio_write_value(int gpio_num, bool level){
    char path[64];
    const char *text;
    int fd;

    //sysfs 的 value 文件只认字符 '0'/'1'，先把电平翻成对应的字符串
    if(level){
        text = "1";
    }
    else{
        text = "0";
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_num);
    fd = open(path, O_WRONLY);
    if(fd == -1){
        perror("open gpio value failed:");
        return;
    }
    if(write(fd, text, 1) == -1){
        perror("write gpio value failed:");
    }
    close(fd);
}

void led_init(void){
    gpio_export_output(LED_GPIO);
    gpio_write_value(LED_GPIO, false);
}

void led_set(bool on){
    gpio_write_value(LED_GPIO, on);
}

void air_con_init(void){
    //分别导出 INA/INB 引脚并配置为输出
    gpio_export_output(AIR_INA_GPIO);
    gpio_export_output(AIR_INB_GPIO);
    /* 两个脚都拉低 = 电机停转。 */
    gpio_write_value(AIR_INA_GPIO, false);
    gpio_write_value(AIR_INB_GPIO, false);
}

void air_con_set(bool on){
    //INA=1、INB=0 正转；INA=0、INB=0 停转
    gpio_write_value(AIR_INA_GPIO, on);
    gpio_write_value(AIR_INB_GPIO, false);
}

// DHT11 温湿度传感器（硬件读取）
void dht11_init(void){
    dht_fd = open("/dev/mydht11_poll", O_RDWR);
    if (dht_fd == -1){
        perror("open dht11 failed");
        return;
    }
    printf("dht11: 已打开 /dev/mydht11_poll (fd = %d)\n", dht_fd);
}

int dht11_read(EnvStatus_t *env){
    char buf[2];
    ssize_t n;

    if (env == NULL || dht_fd == -1){
        return -1;
    }

    /* 驱动一次返回 2 字节：buf[0]=湿度整数, buf[1]=温度整数 */
    n = read(dht_fd, buf, 2);
    if (n != 2){
        return -1;
    }

    env->humi = buf[0];
    env->temp = buf[1];
    return 0;
}
