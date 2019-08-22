#include "Common/Interfaces/state_machine.h"
#include "Common/Interfaces/thread_attr.h"
#include "Common/Interfaces/comm_data_def.h"
#include <string>
#include <ctime>
#include <string.h>
#include"stdlib.h"
#include <errno.h>
#include <iostream>
#include<unistd.h>


//local function
static void* stateMahchineTimeoutStateUpdateLoop(void* data);
static void* stateMahchineDispatchProcessLoop(void* data);
static State executeConditionStateTransform(int state_level,pStateMachine_t pStateMachine,Msg_t msg);
StateUpdateCallback m_stateUpdateCallback;

void actionTrap(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action actionTrap triggered.\n");
}


/**
 * @brief action1 当前状态等级下的当前状态 满足对应条件后执行的函数
 * @param state_level 当前状态等级
 * @param state   当前状态等级下的当前状态
 * @param pStateMachine 状态机
 * @param msg   消息（包含父条件和子条件等信息，不同层次状态转换可根据此转换
 */
void action1(int current_state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 1 triggered.\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_micWrapper->startForCall();   // create  capture audio   mem 需要创建，将自己声音发个对方
#endif
    //2） 执行子条件进入此父状态的子状态转换执行
    printf("%s: GGGGGGGGGGGGGGGGGGGGGGGGGG  msgtype:%d   msg_produce:%d  msg_len=%d msg_src:%d  msg_dst:%d\n",
           __FUNCTION__,
           msg.hdr.msg_type,
           msg.hdr.msg_produce,
           msg.hdr.msg_len,
           msg.hdr.msg_src,
           msg.hdr.msg_dst);

}


void action2(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 2 triggered.\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    sendDataBase(MODULE_ALEXA_CALL,0,ALEXA_CALL_ALERTING);
#endif
}


void action3(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 3 triggered.\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_micWrapper->startForCall();// create  capture audio   mem  需要创建，将自己声音发个对方

    m_interactionManager->acceptCall(); //通知云端alexa Call已连接 ，不管接收端还是发送端收到已连接状态，都需要回放收到对方的声音（即创建回放声音线程）。收到断开状态时也要都释放(回放声音线程)
#endif
}


void action4(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 4 triggered.\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    alexa_call_connected();   //不管任何一方接听，都会收到连接状态（包含本端）,都要开启回放声音线程，
    //  m_playbackThreadToStop = false;
    //m_playbackThread = std::thread(playback_fun, this);
#endif
}


void action5(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 5 triggered.\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    alexa_call_connected();   //不管任何一方接听，都会收到连接状态（包含本端）,都要开启回放声音线程，
    //  m_playbackThreadToStop = false;
    //m_playbackThread = std::thread(playback_fun, this);
#endif

}


void action6(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 6 triggered. only  exit action1 \n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_micWrapper->stopForCall(); // destory  capture audio   mem
#endif
}


void action7(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 7 triggered.   exit action1   action4\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_micWrapper->stopForCall(); // destory  capture audio   mem

    m_playbackThreadToStop = true;
    m_playbackThread.join();
#endif
}


void action8(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 8 triggered. tc  hangup  and  notify  communservices disconnect \n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_interactionManager->stopCall();//通知对端  1）库内部是否发送成功  2）云端是否收到  3）云端是否返回CALL_DISCONNECTED给对端 4）对端收到CALL_DISCONNECTED是否正确处理   5）当再次call上次退出处理流程是否完成   在这几个关键点 打印log
#endif
}



void action9(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 9 triggered.  only  exit action2 \n");
#ifndef  MIMULATED_CONDITION_SWITCH
    sendDataBase(MODULE_ALEXA_CALL,0,ALEXA_CALL_DISCONNECT);
#endif
}



void action10(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 10 triggered.    exit action2  action3\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    sendDataBase(MODULE_ALEXA_CALL,0,ALEXA_CALL_DISCONNECT);

    m_micWrapper->stopForCall(); // destory  capture audio   mem
#endif

}


void action11(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 11 triggered.    exit action2  action3  action5\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    sendDataBase(MODULE_ALEXA_CALL,0,ALEXA_CALL_DISCONNECT);

    m_micWrapper->stopForCall(); // destory  capture audio   mem

    m_playbackThreadToStop = true;
    m_playbackThread.join();
#endif

}


void  action12(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 12 triggered.  notifyNativeCallStarted and  dial  pstn call   \n");
#ifndef  MIMULATED_CONDITION_SWITCH
    if(m_interactionManager)
        m_interactionManager->notifyNativeCallStarted();

    //拨打pstn call
    // Dail
    osal_log(AVS_MODULE_STATUS,OSAL_LOG_INFO, "****SampleApplication::sendDataBase PSTN_STATUS_DIAL, m_phoneNumber: %s", m_phoneNumber);
    sendDataBase(MODULE_SPEECH_RECOGNIZATION,0,ALEXA_PROCEDURE_SKILL_START);

    usleep(2000000);//2000ms

    osal_log(AVS_MODULE_STATUS,OSAL_LOG_INFO, "****SampleApplication::sendDatacontactInfo PSTN_STATUS_DIAL, m_phoneNumber: %s", m_phoneNumber);
    sendDatacontactInfo(m_phoneNumber,m_phoneNumber);

    sendDataBase(MODULE_SPEECH_RECOGNIZATION,0,ALEXA_PROCEDURE_CONVERSATION_STOPPED);
    m_bExpectSpeech = false;
    m_bAudioPlayer_Play = false;
    m_bStopCapture   =  false ;
#endif
    //m_interactionManager->notifyNativeCallStopped();

}



void action13(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 13 triggered.  not deal with\n");
}

void action14(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{

#ifndef  MIMULATED_CONDITION_SWITCH
    printf("Action 14 triggered. notifyIncomingNativeCall=%s\n", m_phoneNumber);
    m_interactionManager->notifyIncomingNativeCall("PSTN Call",m_phoneNumber);     //通知云端接收的联系人信息
#else
    printf("Action 14 triggered. notifyIncomingNativeCall\n");
#endif

}


void action15(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 15 triggered.  notifyNativeCallStopped\n");
#ifndef  MIMULATED_CONDITION_SWITCH
    m_interactionManager->notifyNativeCallStopped();
#endif
}

void action16(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
{
    printf("Action 16 triggered.   release  pstn call \n");
#ifndef  MIMULATED_CONDITION_SWITCH
// 要求tc 释放stn call
    sendDataBase(MODULE_ALEXA_CALL,0,ALEXA_CALL_DISCONNECT);
#endif
}

//////////////////////////////////////////

//跳转关系，把n个跳转加一个陷阱跳转先定义出来
//////////////alexa call////////////////////
//(s1,c1, s2,a1)
Trasition t1 = {
    ALEXA_CALL_STATUS_SEND_START,
    action1
};

//(s2,c3,s5,a4)
Trasition t2 = {
    ALEXA_CALL_STATUS_SEND_CONNECTED,
    action4
};

//(s5,c6,s7,a8)
Trasition t3 = {
    ALEXA_CALL_STATUS_SEND_CONNETED2STOP,
    action8
};



//(s5,c4,s1,a7)
//(s7,c4,s1,a7)
Trasition t4 = {
    CALL_STATUS_IDLE,
    action7
};


//(s2,c6,s9,a8)
Trasition t5 = {
    ALEXA_CALL_STATUS_SEND_START2STOP,
    action8
};


//(s2,c4,s1,a6)
//(s9,c4,s1,a6)
Trasition t6 = {
    CALL_STATUS_IDLE,
    action6
};



//(s1,c2,s3,a2)
Trasition t7 = {
    ALEXA_CALL_STATUS_RECV_START,
    action2
};



//(s3,c5,s4,a3)
Trasition t8 = {
    ALEXA_CALL_STATUS_RECV_ACCEPT,
    action3
};



//(s4,c3,s6,a5)
Trasition t9 = {
    ALEXA_CALL_STATUS_RECV_CONNECTED,
    action5
};


//(s6,c6,s8,a8)
Trasition t10 = {
    ALEXA_CALL_STATUS_RECV_CONNETCED2STOP,
    action8
};


//(s6,c4,s1,a11)
//(s8,c4,s1,a11)
Trasition t11 = {
    CALL_STATUS_IDLE,
    action11
};



//(s4,c6,s11,a8)
Trasition t12 = {
    ALEXA_CALL_STATUS_RECV_ACCEPT2STOP,
    action8
};


//(s4,c4,s1,a10)
//(s11,c4,s1,a10)
Trasition t13 = {
    CALL_STATUS_IDLE,
    action10
};


//(s3,c6,s10,a8)
Trasition t14 = {
    ALEXA_CALL_STATUS_RECV_START2STOP,
    action8
};


//(s3,c4,s1,a9)
//(s10,c4,s1,a9)
Trasition t15 = {
    CALL_STATUS_IDLE,
    action9
};

//////////alexa call///////////////////

/////////pstn  call//////////////////

//(s1,c9,s12,a12)
Trasition t16 = {
    PSTN_CALL_STATUS_SEND_START,
    action12
};
//(s12,c7,s1,a16)
Trasition t17 = {
    CALL_STATUS_IDLE,
    action16
};
//(s12,c6,s1,a15)
Trasition t18 = {
    CALL_STATUS_IDLE,
    action15
};




//(s12,c8,s15,a13)
Trasition t19 = {
    PSTN_CALL_STATUS_SEND_CONNECTED,
    action13
};
//(s15,c6,s1,a15)
Trasition t20 = {
    CALL_STATUS_IDLE,
    action15
};
//(s15,c7,s1,a16)
Trasition t21 = {
    CALL_STATUS_IDLE,
    action16
};



//(s1,c10,s13,a13)
Trasition t22 = {
    PSTN_CALL_STATUS_RECV_START,
    action13
};
//(s13,c6,s1,a15)
Trasition t23 = {
    CALL_STATUS_IDLE,
    action15
};
//(s13,c7,s1,a13)
Trasition t24 = {
    CALL_STATUS_IDLE,
    action13
};



//(s13,c5,s14,a14)
Trasition t25 = {
    PSTN_CALL_STATUS_RECV_ACCEPT,
    action14
};



//(s14,c7,s1,a13)
Trasition t26 = {
    CALL_STATUS_IDLE,
    action13
};


//(s14,c6,s1,a15)
Trasition t27 = {
    CALL_STATUS_IDLE,
    action15
};


/////////pstn  call//////////////////

// (s, c, trap, actionTrap)
Trasition tt = {
    CALL_STATUS_UNKNOW,
    actionTrap
};

//定义跳转表
pTrasition transition_table[STATES][CONDITIONS] = {
    /*      c1,  c2    c3   c4    c5   c6     c7,  c8    c9   c10  */
    /* s1 */&t1, &t7,  &tt, &tt, &tt,  &tt,   &tt, &tt,  &t16,&t22,
    /* s2 */&tt, &tt,  &t2, &t6, &tt,  &t5,   &tt, &tt,  &tt, &tt,
    /* s3 */&tt, &tt,  &tt, &t15, &t8,  &t14, &tt, &tt,  &tt, &tt,
    /* s4 */&tt, &tt,  &t9, &t13, &tt,  &t12, &tt, &tt,  &tt, &tt,
    /* s5 */&tt, &tt,  &tt, &t4, &tt,  &t3,   &tt, &tt,  &tt, &tt,
    /* s6 */&tt, &tt,  &tt, &t11, &tt,  &t10, &tt, &tt,  &tt, &tt,
    /* s7 */&tt, &tt,  &tt, &t4, &tt,  &tt,   &tt, &tt,  &tt, &tt,
    /* s8 */&tt, &tt,  &tt, &t11, &tt,  &tt,  &tt, &tt,  &tt, &tt,
    /* s9 */&tt, &tt,  &tt, &t6, &tt,  &tt,   &tt, &tt,  &tt, &tt,
    /* s10*/&tt, &tt,  &tt, &t15, &tt,  &tt,  &tt, &tt,  &tt, &tt,
    /* s11*/&tt, &tt,  &tt, &t13, &tt,  &tt,  &tt, &tt,  &tt, &tt,

    /* s12*/&tt, &tt,  &tt, &tt, &tt,  &t18,   &t17, &t19,&tt, &tt,
    /* s13*/&tt, &tt,  &tt, &tt, &t25,  &t23,   &t24, &tt, &tt,&tt,
    /* s14*/&tt, &tt,  &tt, &tt, &tt,  &t27,   &t26, &tt,  &tt, &tt,
    /* s15*/&tt, &tt,  &tt, &tt, &tt,  &t20,   &t21, &tt,  &tt, &tt,

    /* st */&tt, &tt,  &tt, &tt, &tt,  &tt,   &tt, &tt,  &tt, &tt,
};

//////////////////




bool initializeStateMachine(pStateMachine_t pStateMachine, State s,StateUpdateCallback stateUpdateCallback)
{
    if(!stateUpdateCallback)
    {
        LOG_RUN_ERROR("stateUpdateCallback  is   null!!!!!");
        return false;
    }
    m_stateUpdateCallback = stateUpdateCallback;

    if(!pStateMachine)
    {
        LOG_RUN_ERROR("pStateMachine  is   null!!!!!");
        return false;
    }
    pStateMachine->current = s;
    int ret = MsgQueueInit(&pStateMachine->msg_queue);
    if(ret != 0)
    {
        LOG_RUN_ERROR("MsgQueueInit pStateMachine->msg_queue  faill!!!");
        return false;
    }


    if(initThreadAttr(&pStateMachine->m_dispatchProcess_thread_attr,THREAD_POLICY,IPC_SEND_HANDLE_THREAD_SCHED_PRIORITY))
    {
        LOG_RUN_DEBUG("initThreadAttr  pStateMachine->m_dispatchProcess_thread_attr  success!!!");
    }
    else
    {
        LOG_RUN_DEBUG("initThreadAttr  pStateMachine->m_dispatchProcess_thread_attr fail!!!");
    }
    pStateMachine->m_bDispatchProcessRunning = true;
    ret = pthread_create(&(pStateMachine->m_dispatchProcess_thread), &pStateMachine->m_dispatchProcess_thread_attr, &stateMahchineDispatchProcessLoop, pStateMachine);  //send
    if(ret  == 0)
    {
        LOG_RUN_DEBUG( "pthread_create m_dispatchProcess_thread  (%s) pStateMachine->m_dispatchProcess_thread=%ld .", strerror(errno),pStateMachine->m_dispatchProcess_thread);
    }
    else
    {
        LOG_RUN_DEBUG( "pthread_create m_dispatchProcess_thread failed (%s).", strerror(errno));
        pStateMachine->m_bDispatchProcessRunning = false;
        return -1;
    }


    if(initThreadAttr(&pStateMachine->m_timeoutStateUpdate_thread_attr,THREAD_POLICY,IPC_SEND_HANDLE_THREAD_SCHED_PRIORITY))
    {
        LOG_RUN_DEBUG("initThreadAttr  pStateMachine->m_timeoutStateUpdate_thread_attr  success!!!");
    }
    else
    {
        LOG_RUN_DEBUG("initThreadAttr  pStateMachine->m_timeoutStateUpdate_thread_attr fail!!!");
    }
    pStateMachine->m_bTimeoutStateUpdateRunning = true;
    ret = pthread_create(&(pStateMachine->m_timeoutStateUpdate_thread), &pStateMachine->m_timeoutStateUpdate_thread_attr, &stateMahchineTimeoutStateUpdateLoop, pStateMachine);  //send
    if(ret  == 0)
    {
        LOG_RUN_DEBUG( "pthread_create m_timeoutStateUpdate_thread  (%s) pStateMachine->m_timeoutStateUpdate_thread=%ld .", strerror(errno),pStateMachine->m_timeoutStateUpdate_thread);
    }
    else
    {
        LOG_RUN_DEBUG( "pthread_create m_timeoutStateUpdate_thread failed (%s).", strerror(errno));
        pStateMachine->m_bTimeoutStateUpdateRunning = false;
        return -1;
    }


    LOG_RUN_VERBOSE("pStateMachine  initialize success!!!");
    return true;
}

bool  destoryStateMachine(pStateMachine_t pStateMachine)
{
    if(!pStateMachine)
    {
        LOG_RUN_ERROR("pStateMachine  is   null!!!!!");
        return false;
    }
    pStateMachine->m_bDispatchProcessRunning = false;
    pStateMachine->m_bTimeoutStateUpdateRunning = false;

    pthread_join(pStateMachine->m_timeoutStateUpdate_thread, NULL);
    LOG_RUN_DEBUG("m_timeoutStateUpdate_thread exit!!!!");

    if(destoryThreadAttr(&pStateMachine->m_timeoutStateUpdate_thread_attr))
    {
        LOG_RUN_DEBUG("destoryThreadAttr   pStateMachine->m_timeoutStateUpdate_thread_attr  success!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("destoryThreadAttr  pStateMachine->m_timeoutStateUpdate_thread_attr  fail!!!!");
        return false;
    }


    pthread_join(pStateMachine->m_dispatchProcess_thread, NULL);
    LOG_RUN_DEBUG("m_dispatchProcess_thread exit!!!!");

    if(destoryThreadAttr(&pStateMachine->m_dispatchProcess_thread_attr))
    {
        LOG_RUN_DEBUG("destoryThreadAttr   pStateMachine->m_dispatchProcess_thread_attr  success!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("destoryThreadAttr  pStateMachine->m_dispatchProcess_thread_attr  fail!!!!");
        return false;
    }


    int ret = MsgQueueDestory(&pStateMachine->msg_queue);
    if(ret != 0)
    {
        LOG_RUN_DEBUG("MsgQueueDestory  fail!!!!!!");
        return false;
    }
    return true;
}

State msgEnStateMachine(pStateMachine_t pStateMachine, Msg_t msg)
{

    MsgEnQueue(&pStateMachine->msg_queue,&msg);
    LOG_RUN_DEBUG("pStateMachine->msg_queue MsgEnQueue  a message!");
}


void* stateMahchineTimeoutStateUpdateLoop(void* data) {
    if(!data) {
        LOG_RUN_DEBUG("data  is  null!!!!");
        return nullptr;
    }
    pStateMachine_t pStateMachine = (pStateMachine_t)data;
    while (pStateMachine->m_bTimeoutStateUpdateRunning) {
        LOG_RUN_DEBUG("UUUUUUUUUUUUUUUUUU");
        m_stateUpdateCallback(STATE_LEVEL_TIMEOUT_UPDATE,pStateMachine->current,pStateMachine);
        sleep(10);//10s
    }
    return nullptr;

}

void* stateMahchineDispatchProcessLoop(void* data) {
    if(!data) {
        LOG_RUN_DEBUG("data  is  null!!!!");
        return nullptr;
    }
    pStateMachine_t pStateMachine = (pStateMachine_t)data;
    while (pStateMachine->m_bDispatchProcessRunning) {
        Msg_t msg;
        memset(&msg, 0 ,sizeof(Msg_t));
        int res = MsgDeQueue(&pStateMachine->msg_queue,&msg); //note : MsgDeQueue include wait sem
        if(res != 0) {
            LOG_RUN_DEBUG("###########################MsgDeQueue fail!!!");
            // sleep(5);
            continue;
        }
        else {
            LOG_RUN_DEBUG("MsgDeQueue success!!!");
            executeConditionStateTransform(STATE_LEVEL_FIRST,pStateMachine,msg);//父状态执行父条件父状态转换   子状态执行子条件子状态转换
            //返回当前等级下 状态转换后的当前的状态
        }

    }
    return nullptr;
}

State executeConditionStateTransform(int current_state_level,pStateMachine_t pStateMachine,Msg_t msg)
{

    if(!pStateMachine)
    {
        LOG_RUN_WARN("pStateMachine  is  null!!!!");
        return CALL_STATUS_UNKNOW;//note
    }


    printf("%s: GGGGGGGGGGGGGGGGGGGGGGGGGG  msgtype:%d   msg_produce:%d  msg_len=%d msg_src:%d  msg_dst:%d\n",
           __FUNCTION__,
           msg.hdr.msg_type,
           msg.hdr.msg_produce,
           msg.hdr.msg_len,
           msg.hdr.msg_src,
           msg.hdr.msg_dst);

    Condition condition = 0;
    State current = pStateMachine -> current;
    pTrasition t =nullptr;
#if 0
    if(current_state_level == STATE_LEVEL_FIRST) //第一等级状态
    {
        //执行父条件父状态转换执行
        condition = msg.hdr.msg_type;
        t = transition_table[current][condition]; //执行父状态跳转表
    }
    else if(current_state_level == STATE_LEVEL_SECOND) //第二等级状态
    {
        //1）根据msg.hdr.msg_type选择对应状态跳转表  需要另外定义子等级状态跳转表，执行对应的函数
        //2)对应等级， 对应类型的子的当前状态获取，可以同个等级，不同类型的子状态同时存在，
        //因此状态机除了要保存第一等级的当前状态，还要保存第二等级，根据msg.hdr.msg_type不同类型下获取当前子状态
        //3)执行条件为condition = msg.hdr.msg_produce
        condition = msg.hdr.msg_produce;

    }
#else

    if(current_state_level == STATE_LEVEL_FIRST) //第一等级状态
    {
        static  int last_msg_produce = CALL_STATUS_CONDITION_UNKNOW;
        static  int last_msg_type =255;
        //执行父条件父状态转换执行
        condition = msg.hdr.msg_produce;

        //判断是否相同消息
        if( (last_msg_type !=  msg.hdr.msg_type) || ((last_msg_type ==  msg.hdr.msg_type) && (last_msg_produce != msg.hdr.msg_produce)) )
        {
            t = transition_table[current][condition]; //执行父状态跳转表
            static int continue_except_count = 0;
            if(t->next == CALL_STATUS_UNKNOW)
            {
                if(continue_except_count > CALL_STATUS_UNKNOW) //判断
                {
                    printf("#############reset idle state. reason: state unknow!!!!!!!!!\n");
                    current = CALL_STATUS_IDLE;
                    pStateMachine->current = current;
                    continue_except_count = 0;
                }

                continue_except_count++;
            }
            else
            {
                continue_except_count = 0;
                (*(t->action))(current_state_level,current,pStateMachine, msg);
                //(int state_level,State state,pStateMachine_t pStateMachine, Msg_t msg)
                LOG_RUN_DEBUG("current  status=%d  condition=%d   executeConditionStateTransform  transition  to  current status=%d",current,condition,t->next);
                current = t->next;
                pStateMachine->current = current;
                m_stateUpdateCallback(STATE_LEVEL_FIRST,pStateMachine->current,pStateMachine);

            }
        }
        last_msg_type =  msg.hdr.msg_type;
        last_msg_produce = msg.hdr.msg_produce;
    }
#endif

    return current;
}





