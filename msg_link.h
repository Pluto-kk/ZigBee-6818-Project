#ifndef MSG_LINK_H
#define MSG_LINK_H

typedef struct msg
{
    char short_addr[4];
    int msg_num;
    int tp_max;
    int tp_min;
    int hd_max;
    int hd_min;
    int tp[10];
    int hd[10];
    struct msg *next;
}MYMSG;

extern MYMSG * link_insert(MYMSG *,char *);
extern MYMSG * arr_insert(MYMSG *,char *,int ,int );


#endif