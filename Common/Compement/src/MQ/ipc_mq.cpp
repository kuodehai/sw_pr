/**
 * @brief  MQ  upline
 * @author wangdaopo
 */
#include "Common/Interfaces/thread_attr.h"
#include "Common/Interfaces/ipc_mq.h"
#include "Common/Interfaces/msg_def.h"
#include "Common/Interfaces/comm_data_def.h"

#include <mqueue.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include<stdio.h>
#include<unistd.h>

///////////////////////////////////////////////
/*Posix消息队列操作函数如下：

1. 创建/获取一个消息队列
mqd_t mq_open(const char *name, int oflag); //专用于打开一个消息队列
mqd_t mq_open(const char *name, int oflag, mode_t mode,
              struct mq_attr *attr);
参数:
   name:  消息队列名字;
   oflag: 与open函数类型, 可以是O_RDONLY, O_WRONLY, O_RDWR, 还可以按位或上O_CREAT, O_EXCL, O_NONBLOCK.
   mode: 如果oflag指定了O_CREAT, 需要指定mode参数;
   attr: 指定消息队列的属性;
返回值:
   成功: 返回消息队列文件描述符;
   失败: 返回-1;
注意-Posix IPC名字限制:
   1. 必须以”/”开头, 并且后面不能还有”/”, 形如:/file-name;
   2. 名字长度不能超过NAME_MAX
   3. 链接时:Link with -lrt（Makefile中使用实时链接库-lrt）
 *
 *
 *
4. 获取/设置消息队列属性
#include    <mqueue.h>
int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
int mq_setattr(mqd_t mqdes, const struct mq_attr *attr, struct mq_attr *attr);
均返回：成功时为0， 出错时为-1
参数:
   newattr: 需要设置的属性
   oldattr: 原来的属性
   每个消息队列有四个属性：
struct mq_attr
{
    long mq_flags;     //message queue flag : 0, O_NONBLOCK
    long mq_maxmsg;     //max number of messages allowed on queue
    long mq_msgsize;    // max size of a message (in bytes)
    long mq_curmsgs;    //number of messages currently on queue
 };

消息队列的限制：
MQ_OPEN_MAX : 一个进程能够同时拥有的打开着消息队列的最大数目
MQ_PRIO_MAX : 任意消息的最大优先级值加1

 6.建立/删除消息到达通知事件
#include    <mqueue.h>
int mq_notify(mqd_t mqdes, const struct sigevent *notification);
返回： 成功时为0，出错时为－1
功能： 给指定队列建立或删除异步事件通知
sigev_notify代表通知的方式: 一般常用两种取值:SIGEV_SIGNAL, 以信号方式通知; SIGEV_THREAD, 以线程方式通知
如果以信号方式通知: 则需要设定一下两个参数:
   sigev_signo: 信号的代码
   sigev_value: 信号的附加数据(实时信号)
如果以线程方式通知: 则需要设定以下两个参数:
   sigev_notify_function
   sigev_notify_attributes

通知方式:
   1. 产生一个信号, 需要自己绑定
   2. 创建一个线程, 执行指定的函数
注意: 这种注册的方式只是在消息队列从空到非空时才产生消息通知事件, 而且这种注册方式是一次性的!
** Posix IPC所特有的功能, System V没有

 */

/*Pthread设计思路:多线程编程设计的主要部分无非是 线程创建，参数传递，数据同步，结果返回以及线程销毁。
https://blog.csdn.net/liujiabin076/article/details/53456962*/


#define UPLINE_IPC_MQ_NAME     "/upline_ipc_mq1"
#define DOWNLINE_IPC_MQ_NAME     "/downline_ipc_mq1"
#define IPC_MQ_MAXMSG   10
#define IPC_MQ_MSGSIZE  1024
//输出调试信息宏
#define LOG_RUN_DEBUG(format,...)  do{\
        printf("DEBUG:[%s:%d][%s] " format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)


//IPC进程间消息队列通道数据结构////////////////////////////////////////
typedef struct {
    bool m_bIPCRecvRunning;
    mqd_t recv_mqd;
    struct mq_attr  recv_setattr;
    struct mq_attr  recv_attr;
    struct sigevent recv_sigev;
    char  recv_buf[IPC_MQ_MSGSIZE];
    IPCRECVCALLBACK m_ipcRecvCallBack;
    pthread_t m_recv_thread;
    pthread_attr_t  m_recv_thread_attr;

    //volatile
    bool m_bIPCSendRunning;
    mqd_t send_mqd;
    pthread_t m_send_thread;
    pthread_attr_t  m_send_thread_attr;

} IPCAgent;

static IPCAgent gIPCAgent;



static void recvNotifyThread(union sigval);
static void* ipcRecvTask(void* data); //recv  thread 进程间消息接收处理者线程
int ipcRecvTaskProduce(void* data);
static void* ipcSendTask(void* data); //send  thread 进程内消息处理者线程，发送消息给另一个进程

static bool  recvMqOpen(const char *recv_ipc_mq_name);
static bool  recvMqClose();

static bool  sendMqOpen(void* data);
static bool  sendMqClose();

bool  recvMqOpen(const char *recv_ipc_mq_name)
{
    memset(&gIPCAgent.recv_setattr, 0, sizeof(gIPCAgent.recv_setattr));
    gIPCAgent.recv_setattr.mq_maxmsg = IPC_MQ_MAXMSG;
    gIPCAgent.recv_setattr.mq_msgsize = IPC_MQ_MSGSIZE;

    gIPCAgent.recv_mqd = mq_open(recv_ipc_mq_name, O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK, 0644, &gIPCAgent.recv_setattr);
    if ((gIPCAgent.recv_mqd < 0) && (errno != EEXIST)) {
        // fprintf(stderr, "mq_open: %s\n", strerror(errno));
        LOG_RUN_DEBUG("mq_open  fail!!!: %s", strerror(errno));
        gIPCAgent.m_bIPCRecvRunning =false;
        return false;
    }

    if ((gIPCAgent.recv_mqd < 0) && (errno == EEXIST)) { // name is exist
        gIPCAgent.recv_mqd = mq_open(recv_ipc_mq_name, O_RDWR  | O_NONBLOCK);
        if (gIPCAgent.recv_mqd < 0) {
            LOG_RUN_DEBUG("mq_open  fail!!!: %s", strerror(errno));
            gIPCAgent.m_bIPCRecvRunning =false;
            return false;
        }
    }
    LOG_RUN_DEBUG("mq_open success!!!!");

    if (mq_getattr(gIPCAgent.recv_mqd, &gIPCAgent.recv_attr) < 0) {
        LOG_RUN_DEBUG( "mq_getattr: %s  recvMqClose!!!! ", strerror(errno));
        recvMqClose();
        return false;
    }
    LOG_RUN_DEBUG("recvMqOpen  success!!!  flags: %ld, maxmsg: %ld, msgsize: %ld, curmsgs: %ld\n",
                  gIPCAgent.recv_attr.mq_flags, gIPCAgent.recv_attr.mq_maxmsg, gIPCAgent.recv_attr.mq_msgsize, gIPCAgent.recv_attr.mq_curmsgs);
    return true;
}

bool  recvMqClose()
{
    LOG_RUN_DEBUG("gIPCAgent.recv_mqd=%d",gIPCAgent.recv_mqd);
    if(gIPCAgent.recv_mqd != -1)
    {
        if(mq_close(gIPCAgent.recv_mqd) == 0)
        {
            LOG_RUN_DEBUG("mq_close success!!");
            gIPCAgent.m_bIPCRecvRunning =false;
        }
        else
        {
            LOG_RUN_DEBUG("mq_close  fail!!: %s", strerror(errno));
            return false;
        }

    }
    return true;
}

bool  sendMqOpen(void* data)
{
    struct mq_attr attr;
    attr.mq_maxmsg = IPC_MQ_MAXMSG;
    attr.mq_msgsize = IPC_MQ_MSGSIZE;
    gIPCAgent.send_mqd = mq_open((const char *)data, O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK, 0644, &attr);
    if ((gIPCAgent.send_mqd < 0) && (errno != EEXIST)) {
        LOG_RUN_DEBUG("mq_open: %s", strerror(errno));
        gIPCAgent.m_bIPCSendRunning = false;
        return false;
    }

    if ((gIPCAgent.send_mqd < 0) && (errno == EEXIST)) { // name is exist
        gIPCAgent.send_mqd = mq_open((const char *)data, O_RDWR  | O_NONBLOCK);
        if (gIPCAgent.send_mqd < 0) {
            LOG_RUN_DEBUG("mq_open: %s", strerror(errno));
            gIPCAgent.m_bIPCSendRunning = false;
            return false;
        }
    }
    LOG_RUN_DEBUG("mq_open success!!!!");

    if (mq_getattr(gIPCAgent.send_mqd, &attr) < 0) {
        LOG_RUN_DEBUG( "mq_getattr: %s  sendMqClose!!!!!! ", strerror(errno));
        sendMqClose();
        return false;
    }
    LOG_RUN_DEBUG("sendMqOpen  success!!! flags: %ld, maxmsg: %ld, msgsize: %ld, curmsgs: %ld\n",
                  attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);

    return true;
}
bool  sendMqClose()
{
    LOG_RUN_DEBUG("gIPCAgent.send_mqd=%d",gIPCAgent.send_mqd);
    if(gIPCAgent.send_mqd != -1)
    {
        if(mq_close(gIPCAgent.send_mqd) == 0)
        {
            LOG_RUN_DEBUG("mq_close success!!");
            gIPCAgent.m_bIPCSendRunning = false;
        }
        else
        {
            LOG_RUN_DEBUG("mq_close  fail!!: %s", strerror(errno));
            return false;
        }
    }
    return true;
}



static void recvNotifyThread(union sigval arg)
{
    LOG_RUN_DEBUG("");
    unsigned int    prio;
    int             recvlen;
    LOG_RUN_DEBUG("recvNotifyThread started\n");
    //关于mq_notify函数 Posix消息队列允许异步事件通知, 以告知何时有一个消息放置到了某个空消息队列中, 以下两种方式可选:
    //1 产生一个信号
    //2 创建一个线程来执行一个指定函数
    mq_notify(gIPCAgent.recv_mqd,&gIPCAgent.recv_sigev);
#if WDP_TEST
    printf(LOCALE_MESSAGE);
#endif
    //   mq_receive函数总是返回指定队列中最高优先级的最早消息, 而且该优先级能随该消息的内容和长度一起返回
    //且mq_receive的len参数的值不能小于能加到所指定队列中的消息的最大大小
    while((recvlen = mq_receive(gIPCAgent.recv_mqd,gIPCAgent.recv_buf,gIPCAgent.recv_attr.mq_msgsize, &prio))>=0)
    {
        printf("recvive-> prio: %d,  read %ld bytes   gIPCAgent.recv_buf: %s\n", prio, (long) recvlen,gIPCAgent.recv_buf);
        gIPCAgent.m_ipcRecvCallBack(gIPCAgent.recv_buf,recvlen);
    }
    if(errno != EAGAIN)
    {
        LOG_RUN_DEBUG("mq_receive: %s", strerror(errno));
        sleep(1);//等待一秒，希望接收缓冲区能得到释放
        return;
    }
    pthread_exit(NULL);
    LOG_RUN_DEBUG("pthread_exit");

}
static void* ipcRecvTask(void* data)
{

    ipcRecvTaskProduce(data);

    return NULL;

}
int ipcRecvTaskProduce(void* data)
{
    if(!data)
    {
        LOG_RUN_DEBUG("data  is  null!!!!");
        return -1;
    }
    pRecv_Agent_t pRecv_Agent = (pRecv_Agent_t)data;
    char *recv_ipc_mq_name = pRecv_Agent->recv_mq_name;
    if(!recv_ipc_mq_name)
    {
        LOG_RUN_DEBUG("recv_ipc_mq_name  is  null!!!!");
        return -1;
    }


    if(!recvMqOpen(recv_ipc_mq_name))
    {
        LOG_RUN_DEBUG("recvMqOpen %s fail!!!!",recv_ipc_mq_name);
        return -1;
    }

    gIPCAgent.recv_sigev.sigev_notify = SIGEV_THREAD;
    gIPCAgent.recv_sigev.sigev_value.sival_ptr = NULL;
    gIPCAgent.recv_sigev.sigev_notify_function = recvNotifyThread; //关于mq_notify函数
    gIPCAgent.recv_sigev.sigev_notify_attributes = NULL;
#if  1
    //关于mq_notify函数 Posix消息队列允许异步事件通知, 以告知何时有一个消息放置到了某个空消息队列中, 以下两种方式可选:
    //1 产生一个信号
    //2 创建一个线程来执行一个指定函数
    if(mq_notify(gIPCAgent.recv_mqd,&gIPCAgent.recv_sigev) == -1)
    {
        LOG_RUN_DEBUG( "mq_notify: %s   recvMqClose!!!!!", strerror(errno));
        //ipcAgentStop();
        recvMqClose();
        //TODO:destory all  only  one run
        return -1;
    }
    LOG_RUN_DEBUG("recvNotifyThread started");
    unsigned int    prio;
    int             recvlen;
    while((recvlen = mq_receive(gIPCAgent.recv_mqd,gIPCAgent.recv_buf,gIPCAgent.recv_attr.mq_msgsize, &prio))>=0)
    {
        LOG_RUN_DEBUG("recvive-> prio: %d,  read %ld bytes   gIPCAgent.recv_buf: %s", prio, (long) recvlen,gIPCAgent.recv_buf);
        gIPCAgent.m_ipcRecvCallBack(gIPCAgent.recv_buf,recvlen);
    }

    if(errno != EAGAIN)
    {
        LOG_RUN_DEBUG("mq_receive: %s", strerror(errno));
        sleep(1);//等待一秒，希望接收缓冲区能得到释放

    }
#endif

    LOG_RUN_DEBUG("while  gIPCAgent.m_bIPCRecvRunning =%d",gIPCAgent.m_bIPCRecvRunning);
    while(gIPCAgent.m_bIPCRecvRunning)
    {

        pause();
        //LOG_RUN_DEBUG("pause() after");
    }

    LOG_RUN_DEBUG("recvMqClose!!!!!!");
    recvMqClose();
    return 0;
}
void* ipcSendTask(void* data)
{
    if(!data)
    {
        LOG_RUN_DEBUG("data  is  null!!!!");
        return nullptr;

    }
    pSend_Agent_t pSend_Agent = (pSend_Agent_t)data;
    char *send_ipc_mq_name = pSend_Agent->send_mq_name;
    if(!send_ipc_mq_name)
    {
        LOG_RUN_DEBUG("send_ipc_mq_name  is  null!!!!");
        return nullptr;
    }

    if(!sendMqOpen(send_ipc_mq_name))
    {
        LOG_RUN_DEBUG("sendMqOpen  %s fail!!!!!",send_ipc_mq_name);
        return nullptr;
    }
    //gIPCAgent.m_bIPCSendRunning = true;
    LOG_RUN_DEBUG("while  gIPCAgent.m_bIPCSendRunning =%d",gIPCAgent.m_bIPCSendRunning);

    while (gIPCAgent.m_bIPCSendRunning) {

        //TODO: wait sem
        Msg_t msg;
        memset(&msg, 0 ,sizeof(Msg_t));
        int res = MsgDeQueue(&pSend_Agent->send_data_queue,&msg); //note : MsgDeQueue include wait sem
        if(res != 0)
        {
            LOG_RUN_DEBUG("###########################MsgDeQueue fail!!!");
            // sleep(5);
            continue;
        }
        else
        {
            LOG_RUN_DEBUG("MsgDeQueue success!!!");

        }

        // msg_printer(&msg);
        long int  ipcRepeatSendSleepms = 5000; //5ms
        while(ipcSendCommand((const char*)&msg,msg.hdr.msg_len,0) != 0  && gIPCAgent.m_bIPCSendRunning  && (ipcRepeatSendSleepms < 5120000))
        {
            LOG_RUN_DEBUG( "ipcSendCommand  fail!!!!");
            usleep(ipcRepeatSendSleepms); //5ms  10ms 20ms   40ms  80ms  160ms 320ms 640ms  1280ms 2560ms 5120ms
            ipcRepeatSendSleepms = ipcRepeatSendSleepms*2;
        }
    }

    LOG_RUN_DEBUG("sendMqClose!!!!!");
    sendMqClose();
    return nullptr;

}

int  ipcAgentStart(void *recv_agent, void *send_agent,IPCRECVCALLBACK ipcRecvCallBack)
{
    int ret;
    if(!ipcRecvCallBack)
    {
        LOG_RUN_DEBUG("ipcRecvCallBack  is  null!!!!!!");
        return -1;
    }
    if(!recv_agent)
    {
        LOG_RUN_DEBUG("recv_agent  is  null!!!!!!");
        return -1;
    }
    if(!send_agent)
    {
        LOG_RUN_DEBUG("send_agent  is  null!!!!!!");
        return -1;
    }

    if(!gIPCAgent.m_bIPCRecvRunning)  //note gIPCAgent.m_recv_thread
    {

        gIPCAgent.m_ipcRecvCallBack = ipcRecvCallBack;

        if(initThreadAttr(&gIPCAgent.m_recv_thread_attr,THREAD_POLICY,IPC_RECV_THREAD_SCHED_PRIORITY))
        {
            LOG_RUN_DEBUG("initThreadAttr  gIPCAgent.m_recv_thread_attr  success!!!");
        }
        else
        {
            LOG_RUN_DEBUG("initThreadAttr  gIPCAgent.m_recv_thread_attr  fail!!!");
        }

        gIPCAgent.m_bIPCRecvRunning = true;
        ret = pthread_create(&(gIPCAgent.m_recv_thread), &gIPCAgent.m_recv_thread_attr, &ipcRecvTask, recv_agent); //recv
        if(ret == 0)
        {
            LOG_RUN_DEBUG( "pthread_create m_recv_thread  success!!! gIPCAgent.m_recv_thread=%ld .",gIPCAgent.m_recv_thread);
        }
        else
        {
            LOG_RUN_DEBUG( "pthread_create m_recv_thread failed (%s).", strerror(errno));
            gIPCAgent.m_bIPCRecvRunning = false;
            return -1;
        }
    }
    else
    {
        LOG_RUN_DEBUG("m_recv_thread  has exist");
    }

    if(!gIPCAgent.m_bIPCSendRunning)   //note gIPCAgent.m_send_thread
    {
        if(initThreadAttr(&gIPCAgent.m_send_thread_attr,THREAD_POLICY,IPC_SEND_HANDLE_THREAD_SCHED_PRIORITY))
        {
            LOG_RUN_DEBUG("initThreadAttr  gIPCAgent.m_send_thread_attr  success!!!");
        }
        else
        {
            LOG_RUN_DEBUG("initThreadAttr  gIPCAgent.m_send_thread_attr  fail!!!");
        }
        gIPCAgent.m_bIPCSendRunning = true;
        ret = pthread_create(&(gIPCAgent.m_send_thread), &gIPCAgent.m_send_thread_attr, &ipcSendTask, send_agent);  //send
        if(ret  == 0)
        {
            LOG_RUN_DEBUG( "pthread_create m_send_thread  (%s) gIPCAgent.m_send_thread=%ld .", strerror(errno),gIPCAgent.m_send_thread);
        }
        else
        {
            LOG_RUN_DEBUG( "pthread_create m_send_thread failed (%s).", strerror(errno));
            gIPCAgent.m_bIPCSendRunning = false;
            return -1;
        }
    }
    else
    {
        LOG_RUN_DEBUG("m_send_thread  has exist");
    }

    LOG_RUN_DEBUG("ipcAgentStart  success!!! \n");
    return 0;
}
int ipcAgentStop(void)
{

    gIPCAgent.m_bIPCRecvRunning = false;
    gIPCAgent.m_bIPCSendRunning = false;

    //if(gIPCAgent.m_bIPCRecvRunning){
    LOG_RUN_DEBUG("reset m_bIPCRecvRunning  pthread_join m_recv_thread");
    gIPCAgent.m_bIPCRecvRunning = false;
    pthread_join(gIPCAgent.m_recv_thread, NULL);
    LOG_RUN_DEBUG("m_recv_thread exit!!!!");
    // }

    //&gIPCAgent.m_recv_thread_attr
    if(destoryThreadAttr(&gIPCAgent.m_recv_thread_attr))
    {
        LOG_RUN_DEBUG("destoryThreadAttr   gIPCAgent.m_recv_thread_attr  success!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("destoryThreadAttr   gIPCAgent.m_recv_thread_attr  fail!!!!");
    }

    // if(gIPCAgent.m_bIPCSendRunning){
    LOG_RUN_DEBUG("reset m_bIPCSendRunning  pthread_join m_send_thread");
    gIPCAgent.m_bIPCSendRunning = false;
    pthread_join(gIPCAgent.m_send_thread, NULL);
    LOG_RUN_DEBUG("m_send_thread exit!!!!");
    // }

    if(destoryThreadAttr(&gIPCAgent.m_send_thread_attr))
    {
        LOG_RUN_DEBUG("destoryThreadAttr   gIPCAgent.m_send_thread_attr  success!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("destoryThreadAttr   gIPCAgent.m_send_thread_attr  fail!!!!");
    }

    LOG_RUN_DEBUG("ipcAgentStop  success!!!!");
    return 0;


}
int ipcSendCommand(const char *sendbuf, int sendlen, int prio)
{
    if (gIPCAgent.send_mqd < 0) {
        LOG_RUN_DEBUG("mq_open: %s", strerror(errno));
        return -1;
    }

    if (mq_send(gIPCAgent.send_mqd, sendbuf, sendlen, prio) == 0)
    {
        LOG_RUN_DEBUG( "mq_send  success!!!!");
    }
    else
    {
        LOG_RUN_DEBUG( "mq_send  fail!!!!: %s", strerror(errno));
        return -1;
    }

    return 0;
}


