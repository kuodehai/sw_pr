#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "local_ipc_data_share_memory.h"

#define SHARE_MEMORY_DEVICE_MAGIC	(0xe15ad300)

#define SMD_DIRECT_IPC_SEMS         1
#define SMD_DIRECT_IPC_SEM_CLIENT   0

typedef struct _local_ipc_data_shm_head
{
    unsigned int 	magic;			/* magic number */
    unsigned int	in; //2^32 = 4 294 967 296
    unsigned int	out;//2^32 = 4 294 967 296
    unsigned int	esize;//frame_num
    unsigned int	size; //frame_size
} local_ipc_data_shm_head_t;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux specific) */
};



#define min(x,y) (x)>(y)?(y):(x)

static int smd_semaphore_create_or_connect(local_ipc_data_shm_t *dmix);

static int smd_semaphore_down(local_ipc_data_shm_t *dmix, int sem_num);

static int smd_semaphore_up(local_ipc_data_shm_t *dmix, int sem_num);

static int smd_semaphore_final(local_ipc_data_shm_t *dmix, int sem_num);

static int smd_semaphore_discard(local_ipc_data_shm_t *dmix);

static int smd_shm_create_or_connect(local_ipc_data_shm_t *dmix);

static int smd_shm_discard(local_ipc_data_shm_t *dmix);


int smd_semaphore_create_or_connect(local_ipc_data_shm_t *dmix)
{
    union semun s;
    struct semid_ds buf;
    int i;

    dmix->semid = semget(dmix->ipc_key, SMD_DIRECT_IPC_SEMS,
                         IPC_CREAT | dmix->ipc_perm);
    if (dmix->semid < 0)
        return -errno;
    if (dmix->ipc_gid < 0)
        return 0;
    for (i = 0; i < SMD_DIRECT_IPC_SEMS; i++) {
        s.buf = &buf;
        if (semctl(dmix->semid, i, IPC_STAT, s) < 0) {
            int err = -errno;
            smd_semaphore_discard(dmix);
            return err;
        }
        buf.sem_perm.gid = dmix->ipc_gid;
        s.buf = &buf;
        semctl(dmix->semid, i, IPC_SET, s);
    }
    return 0;
}

int smd_semaphore_down(local_ipc_data_shm_t *dmix, int sem_num)
{
    struct sembuf op[2] = { { sem_num, 0, 0 }, { sem_num, 1, SEM_UNDO } };
    int err = semop(dmix->semid, op, 2);
    if (err == 0)
        dmix->locked[sem_num]++;
    else if (err == -1)
        err = -errno;
    return err;
}

int smd_semaphore_up(local_ipc_data_shm_t *dmix, int sem_num)
{
    struct sembuf op = { sem_num, -1, SEM_UNDO | IPC_NOWAIT };
    int err = semop(dmix->semid, &op, 1);
    if (err == 0)
        dmix->locked[sem_num]--;
    else if (err == -1)
        err = -errno;
    return err;
}

int smd_semaphore_final(local_ipc_data_shm_t *dmix, int sem_num)
{
    if (dmix->locked[sem_num] != 1) {
        return -EBUSY;
    }
    return smd_semaphore_up(dmix, sem_num);
}

int smd_semaphore_discard(local_ipc_data_shm_t *dmix)
{
    if (dmix->semid >= 0) {
        if (semctl(dmix->semid, 0, IPC_RMID, NULL) < 0)
            return -errno;
        dmix->semid = -1;
    }
    return 0;
}

int smd_shm_create_or_connect(local_ipc_data_shm_t *dmix)
{
    struct shmid_ds buf;
    int tmpid, err, first_instance = 0;

retryget:
    dmix->shmid = shmget(dmix->ipc_key, dmix->shm_size,
                         dmix->ipc_perm);
    if (dmix->shmid < 0 && errno == ENOENT) {
        if ((dmix->shmid = shmget(dmix->ipc_key, dmix->shm_size,
                                  IPC_CREAT | IPC_EXCL | dmix->ipc_perm)) != -1)
            first_instance = 1;
        else if (errno == EEXIST)
            goto retryget;
    }
    err = -errno;
    if (dmix->shmid < 0) {
        if (errno == EINVAL)
            if ((tmpid = shmget(dmix->ipc_key, 0, dmix->ipc_perm)) != -1)
                if (!shmctl(tmpid, IPC_STAT, &buf))
                    if (!buf.shm_nattch)
                        /* no users so destroy the segment */
                        if (!shmctl(tmpid, IPC_RMID, NULL))
                            goto retryget;
        return err;
    }
    dmix->shmptr = shmat(dmix->shmid, 0, 0);
    if (dmix->shmptr == (void *) -1) {
        err = -errno;
        smd_shm_discard(dmix);
        return err;
    }
    mlock(dmix->shmptr, dmix->shm_size);
    if (shmctl(dmix->shmid, IPC_STAT, &buf) < 0) {
        err = -errno;
        smd_shm_discard(dmix);
        return err;
    }
    if (first_instance) {	/* we're the first user, clear the segment */
        memset(dmix->shmptr, 0, dmix->shm_size);
        if (dmix->ipc_gid >= 0) {
            buf.shm_perm.gid = dmix->ipc_gid;
            shmctl(dmix->shmid, IPC_SET, &buf);
        }
        dmix->shmptr->magic = SHARE_MEMORY_DEVICE_MAGIC;
        return 1;
    } else {
        if (dmix->shmptr->magic != SHARE_MEMORY_DEVICE_MAGIC) {
            smd_shm_discard(dmix);
            return -EINVAL;
        }
    }
    return 0;
}

int smd_shm_discard(local_ipc_data_shm_t *dmix)
{
    struct shmid_ds buf;
    int ret = 0;

    if (dmix->shmid < 0)
        return -EINVAL;
    if (dmix->shmptr != (void *) -1 && shmdt(dmix->shmptr) < 0)
        return -errno;
    dmix->shmptr = (void *) -1;
    if (shmctl(dmix->shmid, IPC_STAT, &buf) < 0)
        return -errno;
    if (buf.shm_nattch == 0) {	/* we're the last user, destroy the segment */
        if (shmctl(dmix->shmid, IPC_RMID, NULL) < 0)
            return -errno;
        ret = 1;
    }
    dmix->shmid = -1;
    return ret;
}





int local_ipc_data_shm_open(const char* file,int id,unsigned int frame_size,unsigned int frame_num,local_ipc_data_shm_t* phandle)
{
    int ret, first_instance;
    int fail_sem_loop = 10;
    key_t ipc_key = ftok(file,id);
    if(ipc_key < 0)
    {
        ret = -errno;
        goto _err_nosem;
    }

    phandle->ipc_key	=	ipc_key;
    phandle->ipc_perm	=	0644;
    while(1)
    {
        ret = smd_semaphore_create_or_connect(phandle);
        if(0 != ret)
        {
            goto _err_nosem;
        }
        ret = smd_semaphore_down(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
        if (ret < 0) {
            smd_semaphore_discard(phandle);
            if (--fail_sem_loop <= 0)
                goto _err_nosem;
            continue;
        }
        break;
    }

    phandle->shm_size = sizeof(local_ipc_data_shm_head_t) + frame_size*frame_num;
    first_instance = ret = smd_shm_create_or_connect(phandle);
    if(ret < 0)
    {
        goto _err;
    }

    if(1 == first_instance)
    {
        phandle->shmptr->in		=	0;
        phandle->shmptr->out	=	0;
        phandle->shmptr->size	=	frame_num;
        phandle->shmptr->esize	=	frame_size;
    }
    //printf("in = %u,out = %u\r\n",phandle->shmptr->in,phandle->shmptr->out);
    smd_semaphore_up(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    return 0;
_err:
    if ((phandle->shmid >= 0) && (smd_shm_discard(phandle)))
    {
        if (smd_semaphore_discard(phandle))
            smd_semaphore_final(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    }
    else
    {
        smd_semaphore_up(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    }
_err_nosem:
    return ret;
}

int local_ipc_data_shm_close(local_ipc_data_shm_t* phandle)
{
    smd_semaphore_down(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    if (smd_shm_discard(phandle)) {
        if (smd_semaphore_discard(phandle))
            smd_semaphore_final(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    }
    else
    {
        smd_semaphore_final(phandle, SMD_DIRECT_IPC_SEM_CLIENT);
    }
    phandle = (local_ipc_data_shm_t*)0;
    return 0;
}

int local_ipc_data_shm_read(local_ipc_data_shm_t* phandle,void* buf,int frame_count)
{
    char* shm_data = (char*)&phandle->shmptr[1];
    unsigned int esize = phandle->shmptr->esize;
    smd_semaphore_down(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
    printf("in %d out %d\r\n",phandle->shmptr->in,phandle->shmptr->out);
    if(frame_count > (phandle->shmptr->in - phandle->shmptr->out))
    {
        smd_semaphore_up(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
        return -EAGAIN;
    }
    unsigned int off = phandle->shmptr->out % phandle->shmptr->size; //size:frame_num
    unsigned int l = min(frame_count, phandle->shmptr->size - off);
    //printf("l=%u off=%u\r\n",l,off);
    memcpy(buf, shm_data + off*esize, l*esize); //esize:frame_size
    memcpy((char*)buf + l*esize, shm_data, (frame_count - l)*esize);
    phandle->shmptr->out += frame_count;
    smd_semaphore_up(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
    return frame_count;
}

int local_ipc_data_shm_write(local_ipc_data_shm_t* phandle,const void* buf,int frame_count)
{
    char* shm_data = (char*)&phandle->shmptr[1];
    unsigned int esize = phandle->shmptr->esize;
    smd_semaphore_down(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
    if((phandle->shmptr->size - (phandle->shmptr->in - phandle->shmptr->out)) < frame_count)//当数据被读取慢或没有被读取，当偏移过大phandle->shmptr->size时，out，in重置，这是就主动丢弃没有的数据帧。在异常情况下，读取方挂了，过会再重启，如果偏移超过上限phandle->shmptr->size，就会丢弃
    {
        //phandle->shmptr->out += (frame_count - (phandle->shmptr->size - (phandle->shmptr->in - phandle->shmptr->out)));
        phandle->shmptr->out = phandle->shmptr->in = 0;
    }
    printf("in %d out %d\r\n",phandle->shmptr->in,phandle->shmptr->out);
    unsigned int off = phandle->shmptr->in % phandle->shmptr->size;
    unsigned int l = min(frame_count, phandle->shmptr->size - off);

    memcpy(shm_data + off*esize,buf, l*esize);
    memcpy(shm_data,(char*)buf + l*esize, (frame_count - l)*esize);
    phandle->shmptr->in += frame_count;
    smd_semaphore_up(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
    return frame_count;
}


int local_ipc_data_shm_ioctl(local_ipc_data_shm_t* phandle,unsigned int cmd,void* param)
{
    int ret = 0;
    switch(cmd)
    {
    case LOCAL_IPC_DATA_SHM_CLEAR_BUF_E:
        smd_semaphore_down(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
        phandle->shmptr->in = 0;
        phandle->shmptr->out = 0;
        smd_semaphore_up(phandle,SMD_DIRECT_IPC_SEM_CLIENT);
        break;

    default:
        ret = -EINVAL;
    }
    return ret;
}


