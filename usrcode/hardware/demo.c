#include "gpio.h"
#include "env_status.h"

int main(){
    led_init();
    led_ctrl();

    air_con_init();
    air_con_ctrl();
    
    char temp;
    char humi;
    dht11_init();
    while (1)
    {
        if(dht11_read(&humi,&temp) == 0){
            printf("温度：%u,湿度：%u\n",temp,humi);
        }
        sleep(1);
    }
    return 0;
}