#include "uart_init.h"
#include "msg_link.h"
#include "mqtt.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <mosquitto.h>
#include <time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>


pthread_mutex_t mutex;            //互斥锁

bool control_flag;                //自动手动标志位
struct mosquitto *m_hMqtt;        //结构体传参

int led1_on=0;                    //红灯标志位
int led1_off=0;                    
int led2_on=0;                    //黄灯标志位
int led2_off=0;

int setting_catch();              //设置阈值
int setting_save();               //保存阈值
void* mqtt_pub(void *arg);                                                  

struct mqtt_info
{
    void *arg;
    struct mosquitto *m_hMqtt;
};

struct limit_info *mylimit;         //阈值结构体
struct port_info *myport_info;      //传参结构体

char *usart_dev="/dev/ttyUSB0";     //USB设备名
int usart_fd;                       //串口句柄

/*  unsigned char change(char ch)
    参数：需要改变的字符
    功能:0-f字符转16进制
    返回值：改变后的字符
*/
unsigned char change(char ch)
{
    if(ch>=48&&ch<=57)  //num
        return ch-48;
    else
        return ch-55;  
}
/*void *pth1_callback(void *arg)
    参数：串口句柄
    功能：接收到数据判断是否超过阈值，如果连续超过两次相同，不再写入
    返回值：无
*/
void *pth1_callback(void *arg)
{
    int i=0;
    int uart_fd=*(int *)arg;
    
    while(1)
    {
        char buf[12]={0xfe,0x07,0x01,0x02,0x00,0x0c,0x04,0x0b,0x03,0x03,0x00,0x5c};
        if(myport_info->flag==true )
        {
            buf[4]=change(myport_info->short_addr[0]);
            buf[5]=change(myport_info->short_addr[1]);
            buf[6]=change(myport_info->short_addr[2]);
            buf[7]=change(myport_info->short_addr[3]);
            if(myport_info->shidu>=mylimit->humidity_max || myport_info->shidu<=mylimit->humidity_min)
            {
                if(led1_on<2)
                {
                    led1_on++;
                    led1_off=0;
                    buf[9]=0x01;
                }
            }
            else
            {
                if(led1_off<2)
                {
                    led1_off++;
                    led1_on=0;
                    buf[9]=0x00;
                }
            }
            
            if(myport_info->wendu>=mylimit->temperature_max || myport_info->wendu<=mylimit->temperature_min)
            {
                if(led2_on<2)
                {
                    led2_off=0;
                    led2_on++;
                    buf[8]=0x01;
                }      
            }
            else
            {
                if(led2_off<2)
                {
                    led2_off++;
                    led2_on=0;
                    buf[8]=0x00;
                }
            }
            if(buf[8]!=0x03 || buf[9]!=0x03)
            {
                 pthread_mutex_lock(&mutex);
                 write(uart_fd,buf,sizeof(buf));
                 pthread_mutex_unlock(&mutex);
                 if(control_flag==true)
                {
                    if(buf[8]==0x01 || buf[9]==0x01)
                        buf[10]=0x01;
                    else
                        buf[10]=0x00;
                        buf[2]=0x00;
                    pthread_mutex_lock(&mutex);
                    write(uart_fd,buf,sizeof(buf));
                    pthread_mutex_unlock(&mutex);
                }
            }
            myport_info->flag=false;
        }
    }
}

/*int main()
    主函数
*/
int main() {
    //初始化锁
    pthread_mutex_init(&mutex,NULL);

    control_flag=true;      //自动模式
    MYMSG *mymsg=NULL;      //串口数据存储链表头指针
    mylimit=(struct limit_info *)malloc(sizeof(struct limit_info ));//阈值结构体
    myport_info=(struct port_info *)malloc(sizeof(struct port_info ));//串口实时数据结构体
    //获取阈值
   if(setting_catch()==-1)
   {
       perror("set catch");
       return -1;
   }
   //无名管道 主线程与mqtt发布的线程间通信
    int uart_pub_fd[2];
    int ret=pipe(uart_pub_fd);
    if(ret !=0)
    {
        perror("uart to pub pipe");
        return -1;
    }

    //串口数据数组
    char buf[1024]="";
    //打开串口
    usart_fd=uart_open(&usart_fd,usart_dev);
    if(usart_fd<0)
    {
        perror("usart open");
        return -1;
    }                                                                                
    printf("%d\n",usart_fd);
    //初始化串口
    if(uart_init(usart_fd)==-1)
    {
        perror("usart init");
        return -1;
    }

    pthread_t pth1_id,pth2_id;
    
    pthread_create(&pth1_id,NULL,pth1_callback,&usart_fd);//不断判断是否超过阈值
    pthread_detach(pth1_id);
   
    pthread_create(&pth2_id,NULL,mqtt_pub,&uart_pub_fd);//mqtt订阅、发布线程
    pthread_detach(pth2_id);

    char info_str[1024]="";
    char wendu[128];
    char shidu[128];
    char short_addr[128];
    memset(buf,0,sizeof(buf));
    while(1)
    {   
        memset(buf,0,sizeof(buf));
        //读取串口数据
        int ret=read(usart_fd,buf,sizeof(buf));
        if(ret>0)
        {
           // printf("data=[%s]\n",buf);
            if(strncmp(buf,"ENDDEVICE",9)==0)
            {
                sscanf(buf+10,"ShortAddr:0x%s Temp is:%s Humidity is:%s",short_addr,wendu,shidu);

                char st_buf[1024]="";
                sprintf(st_buf,"%s,%s,%s,%d,%d,%d,%d,%d,%d,",short_addr,wendu,shidu,mylimit->temperature_max,
                        mylimit->temperature_min,mylimit->humidity_max,mylimit->humidity_min,led1_on,led2_on);
                write(uart_pub_fd[1],st_buf,strlen(st_buf));

                memcpy(myport_info->short_addr,short_addr,4);
                myport_info->wendu=atoi(wendu);
                myport_info->shidu=atoi(shidu);
                myport_info->flag=true;
            
                if(mymsg==NULL)
                {
                    //创建串口数据链表头结点
                    mymsg=link_insert(mymsg,myport_info->short_addr);
                }
                //同一段地址数据插入链表
                MYMSG *tmp=arr_insert(mymsg,myport_info->short_addr,myport_info->wendu,myport_info->shidu);

                //新设备数据，创建一个新节点，存放数据
                if(tmp==NULL)
                {
                    printf("NULL\n");
                    link_insert(mymsg,myport_info->short_addr);
                    arr_insert(mymsg,myport_info->short_addr,myport_info->wendu,myport_info->shidu);
                }
                else
                {
                    //同一设备接收到是个数据，去掉最大值、最小值，求平均值，发布到humidity，结构体记录值清零
                    if(tmp->msg_num==10)
                    {
                        printf("%d\n",tmp->msg_num);
                        int i;
                        int tp_sum=0;int hd_sum=0;
                        double tp_avg=0;double hd_avg=0;
                        for(i=0;i<10;i++)
                        {
                            tp_sum+=tmp->tp[i];
                            hd_sum+=tmp->hd[i];
                        }  
                        tp_avg=((tp_sum-tmp->tp_max-tmp->tp_min)*1.0)/8;
                        hd_avg=((hd_sum-tmp->hd_max-tmp->hd_min)*1.0)/8;
                        sprintf(info_str,"%s,%.2lf,%.2lf,",short_addr,tp_avg,hd_avg);
                        mosquitto_publish(m_hMqtt,NULL,"humidity",strlen(info_str)+1,info_str,2,true);
                        tmp->msg_num=0;
                        tmp->hd_max=0;
                        tmp->hd_min=1000;
                        tmp->tp_max=0;
                        tmp->tp_min=1000;
                    }
                }
            }
        }
    }
    return 0;
}

/*void * pth_mqtt_callback(void *arg)
    参数：无名管道读端和m_hMqtt
    功能：从无名管道读取到数据，mqtt发布到data
    返回值：无
*/
void * pth_mqtt_callback(void *arg)
{
    struct mqtt_info *pub_info=(struct mqtt_info *)arg;
    int fd0=*(int *)pub_info->arg;
    char buf[1024]="";
    while(1)
    {
        char topic[128]="";
        memset(buf,0,sizeof(buf));
        read(fd0,buf,sizeof(buf));
        
        mosquitto_publish(pub_info->m_hMqtt,NULL,"data",strlen(buf)+1,buf,2,true);
    }
}

/*void * mqtt_pub(void *arg)
    参数：
    功能：初始化mqtt客户端，订阅kk主题（接收网页端控制命令）
    返回值：无
*/
void* mqtt_pub(void *arg)
{
    int fd0=*(int *)arg;
    
	char *topic2 = "humidity";
	//struct  id_info *myfd=malloc(sizeof(struct  id_info));
   // myfd->uart_fd=uart_fd;
	//初始化lib库函数
	mosquitto_lib_init();
    // 定义一个客户端名为subtest的发布端。客户端表示订阅端的唯一性
    m_hMqtt = mosquitto_new("pubmo", true, "data");

	struct mqtt_info *pub_info=(struct mqtt_info*)malloc(sizeof(struct mqtt_info));
    pub_info->arg=arg;
    pub_info->m_hMqtt=m_hMqtt;

    pthread_t pth1;
    
    pthread_create(&pth1,NULL,pth_mqtt_callback,pub_info);
    pthread_detach(pth1);

	mosquitto_connect_callback_set(m_hMqtt, my_connect_callback);
    mosquitto_disconnect_callback_set(m_hMqtt, my_disconnect_callback);
	//订阅回调
	mosquitto_message_callback_set(m_hMqtt, my_message_callback);
	mosquitto_subscribe_callback_set(m_hMqtt, my_subscribe_callback);
	mosquitto_connect(m_hMqtt, "116.62.37.168", 1883, 10000);
    mosquitto_subscribe(m_hMqtt,NULL,"kk",1);
    printf("连接成功\n");
    mosquitto_loop_start(m_hMqtt);

	/* 阻塞等待 */
	mosquitto_loop_stop(m_hMqtt, false);
	
	mosquitto_destroy(m_hMqtt);
    mosquitto_lib_cleanup();
}

/*int setting_catch()
    参数：无
    功能：设置阈值
    返回值：成功返回0，失败返回-1
*/
int setting_catch()
{
    FILE *fd=fopen("./set_config","r");
    if(fd==NULL)
    {
        perror("open config");
        return -1;
    }
    char buf[1024]="";
    memset(buf,0,sizeof(buf));
    fgets(buf,sizeof(buf),fd);
    sscanf(buf,"%hhd--%hhd--%hhd--%hhd",&mylimit->temperature_max,
                                        &mylimit->temperature_min,
                                        &mylimit->humidity_max,
                                        &mylimit->humidity_min);
    
    fclose(fd);
    return 0;
}

/*int setting_save()
    参数：无
    功能：保存阈值到本地文件set_config
    返回值：成功返回0，失败返回-1
*/
int setting_save()
{
    FILE *fd=fopen("./set_config","w");
    if(fd==NULL)
    {
        perror("open config");
        return -1;
    }
    char buf[1024]="";
    memset(buf,0,sizeof(buf));
    sprintf(buf,"%hhd--%hhd--%hhd--%hhd",mylimit->temperature_max,
                                mylimit->temperature_min,
                                mylimit->humidity_max,
                                mylimit->humidity_min);
    fputs(buf,fd);
    fclose(fd);
    return 0;
}