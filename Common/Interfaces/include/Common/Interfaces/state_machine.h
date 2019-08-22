
/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "Common/Interfaces/msg_def.h"
typedef int State;
typedef int Condition;

#define QUEUE_SIZE  1000
#define   MIMULATED_CONDITION_SWITCH
//如果定义模拟条件宏开关，不执行实际和硬件相关代码，执行硬件相关使用模拟
#define STATES   16
#define CONDITIONS  10
#define STATE_INTRANSACTION  50


//多任务请求状态机定义
typedef struct
{
    State current; //当前状态
    Queue_t msg_queue;//一个用于存储输入消息（包含条件（层次））队列 //Queue_t

    bool m_bDispatchProcessRunning;
    pthread_t m_dispatchProcess_thread;
    pthread_attr_t  m_dispatchProcess_thread_attr;


    bool m_bTimeoutStateUpdateRunning;
    pthread_t m_timeoutStateUpdate_thread;
    pthread_attr_t  m_timeoutStateUpdate_thread_attr;
} StateMachine_t, * pStateMachine_t;


typedef void (*StateUpdateCallback)(int state_level,State state,pStateMachine_t pStateMachine);

//定义跳转类型：
typedef void (*ActionType)(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg);

typedef enum
{
    STATE_LEVEL_FIRST = 0,
    STATE_LEVEL_SECOND,
    STATE_LEVEL_THREE,
    STATE_LEVEL_TIMEOUT_UPDATE,
    STATE_LEVEL_UNKOWN
} STATE_LEVEL_E;

typedef struct
{
    State next;
    ActionType action;
} Trasition, * pTrasition;




//状态枚举
typedef enum
{
    CALL_STATUS_IDLE  =0, //s1
    ALEXA_CALL_STATUS_SEND_START, //s2
    ALEXA_CALL_STATUS_RECV_START, //s3
    ALEXA_CALL_STATUS_RECV_ACCEPT, //s4
    ALEXA_CALL_STATUS_SEND_CONNECTED,//s5
    ALEXA_CALL_STATUS_RECV_CONNECTED, //s6
    ALEXA_CALL_STATUS_SEND_CONNETED2STOP, //s7
    ALEXA_CALL_STATUS_RECV_CONNETCED2STOP,  //s8
    ALEXA_CALL_STATUS_SEND_START2STOP, // s9
    ALEXA_CALL_STATUS_RECV_START2STOP, //s10
    ALEXA_CALL_STATUS_RECV_ACCEPT2STOP,  // s11

    PSTN_CALL_STATUS_SEND_START, //s12
    PSTN_CALL_STATUS_RECV_START,//s13
    PSTN_CALL_STATUS_RECV_ACCEPT,//s14
    PSTN_CALL_STATUS_SEND_CONNECTED,//s15
    CALL_STATUS_UNKNOW

} CALL_STATUS_E;

//跳转条件枚举
typedef enum
{
    //alexa call
    CALL_STATUS_CONDITION_CALL_STATUS_CONNECTING  =0, //c1
    CALL_STATUS_CONDITION_CALL_STATUS_INBOUND_RINGING, //c2
    CALL_STATUS_CONDITION_CALL_STATUS_CONNECTED,  //c3
    CALL_STATUS_CONDITION_CALL_STATUS_DISCONNECTED, //c4


    CALL_STATUS_CONDITION_TC_ANSER, //c5  CALL_STATUS_CONDITION_ALEXA_CALL_ANSER
    CALL_STATUS_CONDITION_TC_DISCONNECT, //c6   CALL_STATUS_CONDITION_ALEXA_CALL_DISCONNECT

    //pstn call
    CALL_STATUS_CONDITION_PSTN_CALL_ON_HANGUP, //c7 收到对方挂掉通知
    CALL_STATUS_CONDITION_PSTN_CALL_ON_ACCPETE, //c8 收到对方接线通知
    CALL_STATUS_CONDITION_PSTN_CALL_ON_NATIVECALL_CONTATC,//c9 收到将要拨打号码

    CALL_STATUS_CONDITION_TC_INCOMING_RING,//c10 todo

    CALL_STATUS_CONDITION_UNKNOW
} CALL_STATUS_CONDITION_E;




#define LOG_RUN_ERROR(format,...)  do{\
        printf("ERROR: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)

#define LOG_RUN_WARN(format,...)  do{\
        printf("WARN: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)


#define LOG_RUN_INFO(format,...)  do{\
        printf("INFO: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)


#define LOG_RUN_DEBUG(format,...)  do{\
        printf("DEBUG: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)

#define LOG_RUN_VERBOSE(format,...)  do{\
        printf("VERBOSE: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)

//api
bool  initializeStateMachine(pStateMachine_t pStateMachine, State s,StateUpdateCallback stateUpdateCallback);
bool  destoryStateMachine(pStateMachine_t pStateMachine);
State msgEnStateMachine(pStateMachine_t pStateMachine, Msg_t msg);

#endif // STATE_MACHINE_H

#ifdef __cplusplus
}
#endif
