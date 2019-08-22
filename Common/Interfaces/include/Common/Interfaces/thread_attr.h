
/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef THREAD_ATTR_H
#define THREAD_ATTR_H

/**
   * @brief initThreadAttr   The incoming thread properties enter the initialization Settings
   * @param thread_attr
   * @param thread_policy
   * @param thread_sched_priority
   * @return
   */
bool initThreadAttr(void *thread_attr, const int thread_policy,const int thread_sched_priority);

bool destoryThreadAttr(void *thread_attr);

#endif // THREAD_ATTR_H



#ifdef __cplusplus
}
#endif

