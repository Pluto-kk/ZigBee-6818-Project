#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>
#include<pthread.h>
struct limit_info
{
    int temperature_max;
    int temperature_min;
    int humidity_max;
    int humidity_min;
};

struct  id_info
{
    int uart_fd;
    int sub_pub_fd;
};

struct port_info
{
    bool flag;
    unsigned char short_addr[4];
    int wendu;
    int shidu;
    unsigned char msg[1024];
};


void my_connect_callback(struct mosquitto *mosq, void *obj, int rc);
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result);
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid,int qos_count,const int *granted_qos);

#endif