
/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COMM_DATA_DEF_H
#define COMM_DATA_DEF_H


#include <sched.h>

const int THREAD_POLICY  = SCHED_FIFO;//线程调度策略为 SCHED_FIFO  实时调度策略，
const int IPC_SEND_THREAD_SCHED_PRIORITY = 99; //调度优先级
const int IPC_SEND_HANDLE_THREAD_SCHED_PRIORITY = 98;
const int IPC_RECV_THREAD_SCHED_PRIORITY = 97;
const int IPC_RECV_HANDLE_THREAD_SCHED_PRIORITY = 96;



#endif // COMM_DATA_DEF_H

#ifdef __cplusplus
}
#endif

