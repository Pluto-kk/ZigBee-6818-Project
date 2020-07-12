#include "msg_link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MYMSG * link_insert(MYMSG *msg,char * str)
{
    if(msg==NULL)
    {
        msg=(MYMSG *)malloc(sizeof(MYMSG));
        msg->tp_max=0;
        msg->tp_min=1000;
        msg->hd_max=0;
        msg->hd_min=1000;
        msg->msg_num=0;
        memcpy(msg->short_addr,str,sizeof(msg->short_addr));
        msg->next=NULL;
    }
    else 
    {
        MYMSG *tmp=msg;
        while(tmp->next!=NULL)
            tmp=tmp->next;
        msg=(MYMSG *)malloc(sizeof(MYMSG));
        msg->tp_max=0;
        msg->tp_min=1000;
        msg->hd_max=0;
        msg->hd_min=1000;
        msg->msg_num=0;
        memcpy(msg->short_addr,str,sizeof(msg->short_addr));
        msg->next=NULL;
        tmp->next=msg;
        msg=tmp;
    }
    return msg;
}

MYMSG * arr_insert(MYMSG *msg,char *str,int tp,int hd)
{
    MYMSG * tmp=msg;
    while(tmp!=NULL)
    {
        if(memcmp(str,tmp->short_addr,4)==0)
            {
                tmp->tp[tmp->msg_num]=tp;
                tmp->hd[tmp->msg_num]=hd;

                if(tmp->tp_max<tp)
                    tmp->tp_max=tp;
                if(tmp->tp_min>tp)
                    tmp->tp_min=tp;
                if(tmp->hd_max<hd)
                    tmp->hd_max=hd;
                if(tmp->hd_min>hd)
                    tmp->hd_min=hd;
                
                tmp->msg_num++;
                return tmp;
            }
            tmp=tmp->next;
    }
    return NULL;
}


