#include <stdio.h>
#include <pthread.h>
#include "data/dev_status.h"
#include "hardware/gpio.h"
#include "hardware/env_status.h"
#include <unistd.h>


static volatile int g_exit = 0;

//线程B：负责每秒钟输出灯具状态
static void *thread_b(void * arg){
    while (!g_exit)
    {
        if (DEVICE_STATUS_ON == get_dev_status(DEVICE_LIGHT)){
            printf("灯具亮着\n");
        }else{
            printf("灯具没亮\n");
        }
        sleep(1);
    }
    return NULL;
}

//线程C：每隔三秒开一次灯
static void * thread_c(void * arg){
    //先操作硬件，在写状态表
    while (!g_exit)
    {
        led_ctrl();
        set_dev_status(DEVICE_LIGHT);
        sleep(3);   
    }
    return NULL;
}

//线程D：每秒输出温湿度数据
static void *thread_d(void * arg){
    EnvStatus_t env;
    while (!g_exit)
    {
        if (dht11_read(&env) == 0){
            printf("温度：%u,湿度：%u\n", env.temp, env.humi);
        }
        sleep(1);
    }
    return NULL;
}

int main(void)
{
    /*main.c
        目标是执行一个任务：每隔三秒开一次灯，且每秒输出灯具的状态
        线程A：主线程，负责创建线程B、C,并进行线程的销毁
        线程B：负责每秒钟输出灯具状态
        线程C：每隔三秒开一次灯
        线程D：每秒钟输出温湿度数据
    */
   //线程A：主线程，负责创建线程B、C、D,并进行线程的销毁
    pthread_t b_tid,c_tid,d_tid;
    led_init();
    dht11_init();
    if(0 != pthread_create(&b_tid,NULL,thread_b,NULL)){
        perror("pthread_b create failed\n");
        return 1;
    }

    if(0 != pthread_create(&c_tid,NULL,thread_c,NULL)){
        perror("pthread_c create failed\n");
        g_exit = 1;
        pthread_join(b_tid,NULL);
        return 1;
    }

    if(0 != pthread_create(&d_tid,NULL,thread_d,NULL)){
        perror("pthread_d create failed\n");
        g_exit = 1;
        pthread_join(b_tid,NULL);
        pthread_join(c_tid,NULL);
        return 1;
    }

    sleep(15);
    g_exit = 1;

    pthread_join(b_tid,NULL);
    pthread_join(c_tid,NULL);
    pthread_join(d_tid,NULL);
    return 0;
}
