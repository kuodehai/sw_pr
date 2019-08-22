/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSG_DEF_H
#define MSG_DEF_H
#include <stdio.h>
#include <semaphore.h>


typedef unsigned int       unit32_t;
typedef unsigned char      unit8_t;

#define MAXDATASIZE  1024
#define QUEUE_SIZE  1000

//进程内消息队列数据结构/////////////////////////////////////

//定义消息结构
/*
msg_type：标记消息类型，当消息接收者看到该msg_type后就知道他要干什么事了
msg_len：消息长度，待扩展，暂时没用到(以后会扩展为变长消息)
msg_src：消息的源地址，即消息的发起者
msg_dst：消息的目的地，即消息的接受者
data[100]:消息除去消息头外可以携带的信息量，定义为100字节
*/

typedef struct Msg_Hdr_s
{
    unit32_t msg_type;
    unit32_t msg_produce;
    unit32_t msg_len;
    unit32_t msg_src;
    unit32_t msg_dst;
} Msg_Hdr_t,*pMsg_Hdr_t;

typedef struct Msg_s
{
    Msg_Hdr_t hdr;
    unit8_t data[MAXDATASIZE];
} Msg_t,*pMsg_t;



//构造循环队列队列可以由链表实现，也可以由数组实现，这里就使用数组实现的循环链表作为我们消息队列的队列模型。
typedef struct Queue_s
{
    unit32_t head;
    unit32_t rear;
    sem_t msg_enqueue_sem;
    sem_t msg_dequeue_sem;
    Msg_t data[QUEUE_SIZE]; //QUEUE_SIZE
} Queue_t,*pQueue_t;

//如果有定义多个队列可以对该队列类外包裹一个队列管理类
int MsgQueueInit(Queue_t* pMsgQueue);
int MsgQueueDestory(Queue_t* pMsgQueue);
int MsgDeQueue(Queue_t* pMsgQueue,Msg_t* msg);
int MsgEnQueue(Queue_t* pMsgQueue,Msg_t* msg);

#endif // MSG_DEF_H

#ifdef __cplusplus
}
#endif

