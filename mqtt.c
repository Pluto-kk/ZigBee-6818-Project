
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "mqtt.h"
extern int setting_save();
extern struct limit_info* mylimit;
extern bool control_flag;
extern int usart_fd;
extern pthread_mutex_t mutex;
extern int led1_on;
extern int led1_off;
extern int led2_on;
extern int led2_off;
// 连接回调函数，当连接成功时会进入这里，可以在这里进行连接成功之后的操作，比如连接之后的信息同步
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	/*char buf[1024]="";
	while(1)
	{
		memset(buf,0,sizeof(buf));
		read(0,buf,sizeof(buf));
	}*/
	printf("connect callback\n");
}
// 断开连接回调函数，在断开连接之后进入
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect close\n");
	printf("%d:\n",__LINE__);
}
// 消息回调
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	int ret = 0,i = 0;
	printf("%s\n",(char *)msg->payload);
	char * str=(char *)msg->payload;
	if(strncmp(str,"set",3)==0)
	{
		sscanf(str,"set %hhd %hhd %hhd %hhd",&mylimit->temperature_max,
									&mylimit->temperature_min,
									&mylimit->humidity_max,
									&mylimit->humidity_min);
		
		if(setting_save()==-1)
		{
			perror("save set");
			return;
		}
		else
		{
			printf("设置成功\n");
		}
	}
	unsigned char buf[12]={0xfe,0x07,0x00,0x02,0x00,0x0c,0x04,0x0b,0x03,0x03,0x00,0x5c};
	if(strcmp(str,"auto")==0)
	{
		control_flag=true;
		printf("cointrol:%d\n",control_flag);
		buf[10]=0;
		pthread_mutex_lock(&mutex);
		write(usart_fd,buf,sizeof(buf));
		pthread_mutex_unlock(&mutex);
		if(led1_on==2)
		{
			buf[10]=0x01;
			pthread_mutex_lock(&mutex);
		 	ret = write(usart_fd,buf,12);
			pthread_mutex_unlock(&mutex);
		}
	}                     
	else if(strcmp(str,"no_auto")==0)
	{
		control_flag=false;
		printf("cointrol:%d\n",control_flag);
	}     
	else if(strcmp(str,"mut_on")==0)
	{
		if(control_flag==false)
		{
			buf[10]=0x01;
			pthread_mutex_lock(&mutex);
		 	ret = write(usart_fd,buf,12);
			pthread_mutex_unlock(&mutex);
		}
	}
	else if(strcmp(str,"mut_off")==0)  
	{
		if(control_flag==false)
		{
			buf[10]=0x00;
			pthread_mutex_lock(&mutex);
		 	ret = write(usart_fd,buf,12);
			/*for(i = 0;i<12;i++)
				printf("----ret :%d --- usart write:0x%x------\n",ret,buf[i]);*/
			pthread_mutex_unlock(&mutex);
		}
	}        
	printf("success\n");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
}
// 订阅消息回调
void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid,int qos_count,const int *granted_qos)
{
	printf("my sub message\n");
	int i;
	time_t t;
	struct tm *lt;
	time(&t);
	lt =  localtime(&t);
	printf("%d: %s\n",__LINE__,(char *)obj);
	printf("%d: mid=%d\n",__LINE__,mid);
	printf("%d: qos_count=%d\n",__LINE__,qos_count);
	for(i=0;i<qos_count;i++)
		printf("%d: granted_qos[%d]=%d\n",__LINE__,i,granted_qos[i]);
}
