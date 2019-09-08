#include <fcntl.h>
#include <stdlib.h>
#include <sys/ipc.h>

#define SMD_DIRECT_IPC_SEMS         1

typedef struct _local_ipc_data_shm_head local_ipc_data_shm_head_t;

typedef struct _local_ipc_data_shm
{
    key_t 			ipc_key;			/* IPC key for semaphore and memory */
    mode_t 			ipc_perm;		/* IPC socket permissions */
    int 			ipc_gid;			/* IPC socket gid */
    int 			semid;			/* IPC global semaphore identification */
    int 			locked[SMD_DIRECT_IPC_SEMS];	/* local lock counter */
    int 			shmid;			/* IPC global shared memory identification */
    unsigned int 	shm_size; // sizeof(local_ipc_data_shm_head_t) + frame_size*frame_num;
    local_ipc_data_shm_head_t* 	shmptr;	/* pointer to shared memory area */
} local_ipc_data_shm_t;

typedef enum
{
    LOCAL_IPC_DATA_SHM_CLEAR_BUF_E,
} LOCAL_IPC_DATA_SHM_IOCTL_CMD;


int local_ipc_data_shm_open(const char* file,int id,unsigned int frame_size,unsigned int frame_num,local_ipc_data_shm_t* phandle);

int local_ipc_data_shm_close(local_ipc_data_shm_t* phandle);

int local_ipc_data_shm_read(local_ipc_data_shm_t* phandle,void* buf,int frame_count);

int local_ipc_data_shm_write(local_ipc_data_shm_t* phandle,const void* buf,int frame_count);

int local_ipc_data_shm_ioctl(local_ipc_data_shm_t* phandle,unsigned int cmd,void* param);


