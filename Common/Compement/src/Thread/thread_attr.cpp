#include "Common/Interfaces/thread_attr.h"

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>


bool setThreadPriority(void *thread_attr,const int thread_policy,const void *thread_param)
{
    int ret =- 1;
    if(thread_attr == nullptr)
    {
        printf("thread_attr  is  null!!!!!!!!!!");
        return  false;
    }
    pthread_attr_t *attr = (pthread_attr_t *)thread_attr;
    struct sched_param *param = ( struct sched_param *)thread_param;
    if(param == nullptr)
    {
        printf("param  is  null   destoryThreadAttr!!!!!!!!!!");
        destoryThreadAttr(attr);
        return  false;
    }
//  获取 调度策略 最大和最小优先级
    int   priority_max  =sched_get_priority_max(thread_policy);
    int   priority_min  =sched_get_priority_min(thread_policy);
//判断想要设置的优先级是否超过调度策略的优先级   优先级设置超过设置范围
    if(param->sched_priority > priority_max  || param->sched_priority < priority_min )
    {
        printf("the  thread  priority setting exceeds the setting range    destoryThreadAttr!!!!!!!!!");
        destoryThreadAttr(attr);
        return false;
    }

//设置处理线程调度策略
    ret =pthread_attr_setschedpolicy(attr,thread_policy);
    if(ret == 0)
    {
        printf("pthread_attr_setschedpolicy success!!!!!!!!!");
    }
    else
    {
        printf("pthread_attr_setschedpolicy fail   destoryThreadAttr!!!!!!!!!!");
        destoryThreadAttr(attr);
        return  false;
    }
//判断策略是否设置成功
    int tmp_policy ;
    ret = pthread_attr_getschedpolicy (attr, &tmp_policy);
    if(ret == 0  &&  tmp_policy == thread_policy)
    {
        printf("pthread_attr_getschedpolicy  thread_policy=%d  success!!!!!!!!! ",thread_policy);
    }
    else
    {
        printf("pthread_attr_getschedpolicy  thread_policy=%d  fail    destoryThreadAttr!!!!!!!!!! ",thread_policy);
        destoryThreadAttr(attr);
        return  false;
    }


//设置控制指令处理线程优先级
//param.sched_priority = sched_priority;
    ret =  pthread_attr_setschedparam(attr,param);
    if(ret == 0)
    {
        printf("pthread_attr_setschedparam success!!!!!!!!!");
    }
    else
    {
        printf("pthread_attr_setschedparam fail    destoryThreadAttr!!!!!!!!!!");
        destoryThreadAttr(attr);
        return  false;
    }
//判断设置的优先级是否成功
    struct sched_param tmp_param;
    ret = pthread_attr_getschedparam(attr,&tmp_param);
    if(ret == 0  &&  tmp_param.sched_priority == param->sched_priority)
    {
        printf("pthread_attr_getschedparam   param->sched_priority=[%d]   success!!!!!!!!!", param->sched_priority);
    }
    else
    {
        printf("pthread_attr_getschedparam  param->sched_priority=%d   fail   destoryThreadAttr!!!!!!!!!!  ",param->sched_priority);
        destoryThreadAttr(attr);
        return  false;
    }
    return  true;
}


bool initThreadAttr(void *thread_attr, const int thread_policy,const int thread_sched_priority)
{
    struct sched_param thread_param;
    thread_param.sched_priority = thread_sched_priority; //调度优先级
    if(thread_attr == nullptr)
    {
        printf("thread_attr  is  null!!!!!!!!!!");
        return  false;
    }

    pthread_attr_t *attr = (pthread_attr_t *)thread_attr;
    /*       * 对线程属性初始化      * 初始化完成以后，pthread_attr_t 结构所包含的结构体      * 就是操作系统实现支持的所有线程属性的默认值      */
    int ret = pthread_attr_init (attr);
    if(ret == 0)
    {
        printf("pthread_attr_init success!!!!!!!!!");
    }
    else
    {
        printf("pthread_attr_init fail!!!!!!!!!!");
        return  false;
    }

    bool bPriority  = setThreadPriority(attr,thread_policy,&thread_param);
    if (bPriority)
    {
        printf("setThreadPriority  thread_policy=%d   thread_param.sched_priority=[%d]  success!!!!!!!!!", thread_policy,thread_param.sched_priority);
    }
    else
    {
        printf("setThreadPriority   thread_policy=%d   thread_param.sched_priority=[%d]     fail!!!!!!!!!!",thread_policy,thread_param.sched_priority);
        return  false;
    }
    return  true;
}


bool destoryThreadAttr(void *thread_attr)
{

    /*       * 反初始化 pthread_attr_t 结构      * 如果 pthread_attr_init 的实现对属性对象的内存空间是动态分配的，      * phread_attr_destory 就会释放该内存空间      */

    if(thread_attr == nullptr)
    {
        printf("thread_attr  is  null!!!!!!!!!!");
        return  false;
    }

    int ret = pthread_attr_destroy ((pthread_attr_t *)thread_attr);
    if(ret == 0)
    {
        printf("pthread_attr_destroy success!!!!!!!!!");
    }
    else
    {
        printf("pthread_attr_destroy fail!!!!!!!!!!");
        return  false;
    }
    return  true;
}

