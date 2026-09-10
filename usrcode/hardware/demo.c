#include "gpio.h"
#include "env_status.h"

int main(){
    printf("== demo start ==\n");
    led_init();
    led_set(true);

    // air_con_init();
    // air_con_set(true);
    
    EnvStatus_t env;
    dht11_init();
    while (1)
    {
        if(dht11_read(&env) == 0){
            printf("温度：%u,湿度：%u\n",env.temp,env.humi);
        }
        sleep(2);
    }
    return 0;
}