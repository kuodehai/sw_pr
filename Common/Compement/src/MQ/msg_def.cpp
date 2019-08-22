#include "Common/Interfaces/msg_def.h"
#include <string.h>
#include"stdlib.h"

#include <errno.h>

/*
循环队列的实现想必大家都比较熟悉，但这里需要提示的几点是：
队列中应加入信号量或锁来保证进队时的互斥访问，因为多个消息可能同时进队，互相覆盖其队列节点
这里的信号量仅用于进队而没用于出队，理由是消息处理者只有一个，不存在互斥的情形
*/

//输出调试信息宏
#define LOG_RUN_DEBUG(format,...)  do{\
        printf("DEBUG:[%s:%d][%s] " format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)



//note: only one  msgqueue  ,if multi msgqueue need  Queue_t* pMsgQueue []
//如果是类将此变量移到头文件中 ，如果有定义多个队列可以对该队列类外包裹一个队列管理类



int MsgQueueInit(Queue_t* pMsgQueue)
{

#if  0
    QUEUE_SIZE = max_queue_size;
    pMsgQueue = (struct Queue_s *)malloc(sizeof( struct Queue_s));
    if(!pMsgQueue)
    {
        LOG_RUN_DEBUG("Invalid pMsgQueue!");
        return -1;
    }
    else
    {
        LOG_RUN_DEBUG("malloc pMsgQueue  success!!!!");
    }
    pMsgQueue->data = (struct Msg_s *)malloc(sizeof( struct Msg_s)*max_queue_size);
    if(!pMsgQueue->data)
    {
        LOG_RUN_DEBUG("Invalid pMsgQueue->data!");
        return -1;
    }
    else
    {
        LOG_RUN_DEBUG("malloc   pMsgQueue->data   max_queue_size=%d  success !!!!",max_queue_size);
    }

#endif

    if(!pMsgQueue)
    {
        LOG_RUN_DEBUG("Invalid pMsgQueue!");
        return -1;
    }
    pMsgQueue->rear = 0;
    pMsgQueue->head = 0;
    if(sem_init(&pMsgQueue->msg_enqueue_sem, 0, 1)==0)
    {
        LOG_RUN_DEBUG("sem_init msg_enqueue_sem success!!!!!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("sem_init msg_enqueue_sem  %s", strerror(errno));
        return -1;
    }
    // int sem_init __P ((sem_t *__sem, int __pshared, unsigned int __value));
    //sem为指向信号量结构的一个指针；pshared不为０时此信号量在进程间共享，否则只能为当前进程的所有线程共享；value给出了信号量的初始值

    if(sem_init(&pMsgQueue->msg_dequeue_sem, 0, 1)==0)
    {
        LOG_RUN_DEBUG("sem_init msg_dequeue_sem success!!!!!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("sem_init msg_dequeue_sem  %s", strerror(errno));
        return -1;
    }

    LOG_RUN_DEBUG("MsgQueueInit   success!!!!\n");
    return 0;
}

int MsgQueueDestory(Queue_t* pMsgQueue)
{
    if(!pMsgQueue)
    {
        LOG_RUN_DEBUG("Invalid pMsgQueue!");
        return -1;
    }
    if(sem_destroy(&pMsgQueue->msg_enqueue_sem) == 0)
    {
        LOG_RUN_DEBUG("sem_destroy  msg_enqueue_sem  success!!!");
    }
    else
    {
        LOG_RUN_DEBUG("sem_destroy msg_enqueue_sem %s", strerror(errno));
        return -1;
    }

    if(sem_destroy(&pMsgQueue->msg_dequeue_sem) == 0)
    {
        LOG_RUN_DEBUG("sem_destroy  msg_dequeue_sem  success!!!");
    }
    else
    {
        LOG_RUN_DEBUG("sem_destroy msg_dequeue_sem %s", strerror(errno));
        return -1;
    }

    //函数sem_destroy(sem_t *sem)用来释放信号量sem。
    pMsgQueue->rear = 0;
    pMsgQueue->head = 0;

    LOG_RUN_DEBUG("MsgQueueDestory   success!!!!!");
    return 0;
}

int MsgDeQueue(Queue_t* pMsgQueue,Msg_t* msg)
{
    if(!pMsgQueue)
    {
        LOG_RUN_DEBUG("Invalid pMsgQueue!!!");
        return -1;
    }

    sem_wait(&pMsgQueue->msg_dequeue_sem);
    if(pMsgQueue->rear == pMsgQueue->head) //only one consumer,no need to lock head
    {
        LOG_RUN_DEBUG("Empty pMsgQueue!!!");
        return -1;
    }
    memcpy(msg, &(pMsgQueue->data[pMsgQueue->head]), sizeof(Msg_t));
    pMsgQueue->head = (pMsgQueue->head+1)%QUEUE_SIZE;

    return 0;

}

int MsgEnQueue(Queue_t* pMsgQueue,Msg_t* msg)
{
    if(!pMsgQueue)
    {
        printf("Invalid pMsgQueue!\n");
        return -1;
    }
    if(pMsgQueue->head == (pMsgQueue->rear+1)%QUEUE_SIZE)
    {
        printf("Full pMsgQueue!\n");
        return -1;
    }
    sem_wait(&pMsgQueue->msg_enqueue_sem);
    //函数sem_wait( sem_t *sem )被用来阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一，表明公共资源经使用后减少。函数sem_trywait ( sem_t *sem )是函数sem_wait（）的非阻塞版本，它直接将信号量sem的值减一。
    //如果有两个线程都在sem_wait()中等待同一个信号量变成非零值，那么当它被第三个线程增加 一个“1”时，等待线程中只有一个能够对信号量做减法并继续执行，另一个还将处于等待状态。
    LOG_RUN_DEBUG("");

    memcpy(&(pMsgQueue->data[pMsgQueue->rear]), msg, sizeof(Msg_t));
    pMsgQueue->rear = (pMsgQueue->rear+1)%QUEUE_SIZE;

    sem_post(&pMsgQueue->msg_enqueue_sem);
    sem_post(&pMsgQueue->msg_dequeue_sem);
    //函数sem_post( sem_t *sem )用来增加信号量的值。当有线程阻塞在这个信号量上时，调用这个函数会使其中的一个线程不在阻塞，选择机制同样是由线程的调度策略决定的。
    return 0;
}



