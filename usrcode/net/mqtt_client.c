#include "mqtt_client.h"

#include "data/dev_status.h"
#include "data/env_status.h"
#include "hardware/gpio.h"

#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//配置
#define MQTT_BROKER_HOST    "broker.hivemq.com"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "zaojun_imx6ull_board"
#define MQTT_KEEPALIVE_S    30
#define MQTT_TOPIC_PREFIX   "home/zaojun_imx6ull"

#define TOPIC_CMD_SUB       MQTT_TOPIC_PREFIX "/cmd/#"
#define TOPIC_CMD_LED       MQTT_TOPIC_PREFIX "/cmd/led"
#define TOPIC_CMD_AC        MQTT_TOPIC_PREFIX "/cmd/ac"
#define TOPIC_ST_LED        MQTT_TOPIC_PREFIX "/status/led"
#define TOPIC_ST_AC         MQTT_TOPIC_PREFIX "/status/ac"
#define TOPIC_ST_ENV        MQTT_TOPIC_PREFIX "/status/env"

#define ENV_REPORT_INTERVAL_S   5   // 温湿度上报周期

/* ============================ 全局状态 ============================ */
static struct mosquitto *g_mosq = NULL;      /* 客户端句柄   */
static pthread_t         g_report_tid;       /* 周期上报线程 */
static volatile int      g_running = 0;      /* 1 = 运行中  */
static int               g_report_started = 0;

/* 把载荷判断为“开”。只认 on / 1，其余一律当“关”，避免误开。 */
static int payload_is_on(const void *data, int len)
{
    if (len == 2 && memcmp(data, "on", 2) == 0) {
        return 1;
    }
    if (len == 1 && memcmp(data, "1", 1) == 0) {
        return 1;
    }
    return 0;
}

/* ============================ 状态上报 ============================ */

static void publish_led_status(void)
{
    const char *p = (get_dev_status(DEVICE_LIGHT) == DEVICE_STATUS_ON) ? "on" : "off";
    mosquitto_publish(g_mosq, NULL, TOPIC_ST_LED, (int)strlen(p), p, 1, 1);
}

static void publish_ac_status(void)
{
    const char *p = (get_dev_status(DEVICE_AIRCONDITIONER) == DEVICE_STATUS_ON) ? "on" : "off";
    mosquitto_publish(g_mosq, NULL, TOPIC_ST_AC, (int)strlen(p), p, 1, 1);
}

static void publish_env_status(void)
{
    EnvStatus_t env;
    char buf[64];

    if (get_env_status(&env) != 0) {
        return;             
    }
    snprintf(buf, sizeof(buf), "{\"temp\":%d,\"humi\":%d}", (int)env.temp, (int)env.humi);
    mosquitto_publish(g_mosq, NULL, TOPIC_ST_ENV, (int)strlen(buf), buf, 0, 1);
}

/* ============================ libmosquitto 回调 ============================ */

static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    (void)userdata;

    if (rc != 0) {
        printf("mqtt: 连接被拒绝 rc=%d (%s)\n", rc, mosquitto_connack_string(rc));
        return;
    }

    printf("mqtt: 已连接 %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);

    /* 订阅控制主题（qos=1，和设备端 publish 的 qos 对应） */
    mosquitto_subscribe(mosq, NULL, TOPIC_CMD_SUB, 1);

    publish_led_status();
    publish_ac_status();
    publish_env_status();
}

static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    (void)mosq;
    (void)userdata;

    if (rc != 0) {
        printf("mqtt: 与 Broker 断开 rc=%d，等待自动重连\n", rc);
    }
}

static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg)
{
    (void)mosq;
    (void)userdata;

    if (msg->payloadlen <= 0) {
        return;
    }
    printf("mqtt: 收到 %s = %.*s\n", msg->topic, msg->payloadlen, (char *)msg->payload);

    if (strcmp(msg->topic, TOPIC_CMD_LED) == 0) {
        int on = payload_is_on(msg->payload, msg->payloadlen);
        led_set(on);               
        set_dev_status(DEVICE_LIGHT, on ? DEVICE_STATUS_ON : DEVICE_STATUS_OFF);
        publish_led_status();   
    } else if (strcmp(msg->topic, TOPIC_CMD_AC) == 0) {
        int on = payload_is_on(msg->payload, msg->payloadlen);
        air_con_set(on);
        set_dev_status(DEVICE_AIRCONDITIONER, on ? DEVICE_STATUS_ON : DEVICE_STATUS_OFF);
        publish_ac_status();
    }
}

/* ============================ 周期上报线程 ============================ */

static void *env_report_thread(void *arg)
{
    (void)arg;

    while (g_running) {
        publish_env_status();

        for (int i = 0; i < ENV_REPORT_INTERVAL_S * 10 && g_running; i++) {
            usleep(100 * 1000);
        }
    }
    return NULL;
}

/* ============================ 对外接口 ============================ */

int mqtt_client_start(void)
{
    int rc;

    mosquitto_lib_init();

    /* clean_session=true：不保留会话状态，每次连上都是干净的订阅 */
    g_mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
    if (g_mosq == NULL) {
        printf("mqtt: mosquitto_new 失败\n");
        mosquitto_lib_cleanup();
        return -1;
    }

    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_disconnect_callback_set(g_mosq, on_disconnect);
    mosquitto_message_callback_set(g_mosq, on_message);

    /* 用异步连接：即使此刻网络还没就绪也不会卡住主线程 */
    rc = mosquitto_connect_async(g_mosq, MQTT_BROKER_HOST, MQTT_BROKER_PORT,
                                 MQTT_KEEPALIVE_S);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("mqtt: connect_async 失败 rc=%d\n", rc);
    }

    rc = mosquitto_loop_start(g_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("mqtt: loop_start 失败 rc=%d\n", rc);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        mosquitto_lib_cleanup();
        return -1;
    }

    g_running = 1;
    if (pthread_create(&g_report_tid, NULL, env_report_thread, NULL) == 0) {
        g_report_started = 1;
    } else {
        printf("mqtt: 上报线程创建失败\n");
    }

    printf("mqtt: 客户端已启动，Broker=%s:%d，订阅 %s\n",
           MQTT_BROKER_HOST, MQTT_BROKER_PORT, TOPIC_CMD_SUB);
    return 0;
}

void mqtt_client_stop(void)
{
    g_running = 0;

    if (g_report_started) {
        pthread_join(g_report_tid, NULL);
        g_report_started = 0;
    }

    if (g_mosq != NULL) {
        mosquitto_disconnect(g_mosq);
        mosquitto_loop_stop(g_mosq, true);   /* true: 强制退出 loop 线程 */
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }

    mosquitto_lib_cleanup();
    printf("mqtt: 客户端已停止\n");
}
