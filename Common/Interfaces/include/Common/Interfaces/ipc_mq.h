/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPC_MQ_H
#define IPC_MQ_H

#include "Common/Interfaces/msg_def.h"

#define MAX_MQ_MAME_LEN 1024
typedef  void(*IPCRECVCALLBACK)(char *data,int data_Len);
typedef struct  Send_Agent_s
{
    char send_mq_name[MAX_MQ_MAME_LEN];
    Queue_t send_data_queue;
} Send_Agent_t,*pSend_Agent_t;


typedef struct  Recv_Agent_s
{
    char recv_mq_name[MAX_MQ_MAME_LEN];
    Queue_t recv_data_queue;
} Recv_Agent_t,*pRecv_Agent_t;



//api
int ipcAgentStart(void *recv_agent, void *send_agent,IPCRECVCALLBACK ipcRecvCallBack);//进程间消息生产者或消费者
int ipcAgentStop(void);
int ipcSendCommand(const char *sendbuf, int sendlen, int prio);//进程内消息生产者
#endif // IPC_MQ_H

#ifdef __cplusplus
}
#endif


