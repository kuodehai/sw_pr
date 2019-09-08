/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include "local_ipc_data_share_memory.h"
#define FRAME_NUM      10
#define IS_ADD_LOCAL_IPC_DATA_HEAD  1

#if IS_ADD_LOCAL_IPC_DATA_HEAD
typedef enum {
    MEDIA_TYPE_PCM = 1,
    MEDIA_TYPE_OPUS,
    MEDIA_TYPE_G711,
    MEDIA_TYPE_G722,
    MEDIA_TYPE_MP3,
    MEDIA_TYPE_H264,
    MEDIA_TYPE_H265
}
MEDIA_TYPE_E;

typedef struct local_ipc_data_head
{
    unsigned short media_type;
    unsigned short current_sequence;
    unsigned long current_timestamp;

    //media head infomation
    unsigned int  local_ipc_data_size;
} local_ipc_data_head_t,*plocal_ipc_data_head_t;

const  unsigned int LOCAL_IPC_DATA_HEAD_SIZE =  sizeof(local_ipc_data_head_t);
#endif
unsigned long gettimeofdayUsec(void)
{
    struct timeval stamp;
    gettimeofday(&stamp, NULL);
    return (stamp.tv_sec*1000000 + stamp.tv_usec);
}


int main(int argc, char **argv)
{
    /**
    *device id
    */
    unsigned int  device_id =1;

    /**
    *localipc_data   output buffer
    */
    unsigned char*	local_ipc_data_output_buffer;


    /**
    *localipc_data output share memory
    */

    local_ipc_data_shm_t		shm;

    int max_payload_bytes =  44100*2;
    int payload_count = 1; //media data  write sharemem 1 frame   payload_count =1

    unsigned int frame_size =max_payload_bytes;
#if IS_ADD_LOCAL_IPC_DATA_HEAD
    frame_size += LOCAL_IPC_DATA_HEAD_SIZE;
#endif

    unsigned int local_ipc_data_output_buffer_size = payload_count * max_payload_bytes;
#if IS_ADD_LOCAL_IPC_DATA_HEAD
    local_ipc_data_output_buffer_size += LOCAL_IPC_DATA_HEAD_SIZE;//opus head infomation
#endif
    local_ipc_data_output_buffer	=	calloc(1, local_ipc_data_output_buffer_size);
    if (!local_ipc_data_output_buffer)
        return -1;


    int err = local_ipc_data_shm_open("/tmp", device_id,frame_size, FRAME_NUM,&shm);
    if(0 != err)
    {
        return err;
    }

    //local_ipc_data_shm_ioctl(&shm,LOCAL_IPC_DATA_SHM_CLEAR_BUF_E,NULL);
#if 0  /////////a-z//////////

    int   j ='a';
    while(1)
    {

        if(j > 'z')
        {
            j= 'a';
        }
        memset(local_ipc_data_output_buffer, j, max_payload_bytes);

        int res = local_ipc_data_shm_write(&shm,local_ipc_data_output_buffer,1);
        if(res != 1)
        {
            printf("write error %d\r\n",res);
        }

        usleep(1000000);//10ms
        j++;
    }

#else//read file to  send  data

    const char* filename = "./music_mp3_128.mp3"; //frame_size 44100*2
    FILE *fp = NULL;
    fp = fopen(filename, "ab+");
    if(fp == NULL)
    {
        return 0;
    }

    size_t  ret = 0;
    static unsigned int s_u32_frame_count =0;
    static unsigned long s_u64_last_timestamp = 0;
    unsigned long current_timestamp = gettimeofdayUsec();
    if(s_u64_last_timestamp == 0)
    {
        s_u64_last_timestamp = current_timestamp;
    }
#if IS_ADD_LOCAL_IPC_DATA_HEAD
    static unsigned short s_u16_media_type = 0;
    static unsigned short  s_u16_current_sequence = 0;
#endif
    do
    {
        //fread(buf, sizeof(buf的数据类型), 帧写入长度, pcm_file);
        s_u32_frame_count++;
#if IS_ADD_LOCAL_IPC_DATA_HEAD
        plocal_ipc_data_head_t plocal_ipc_data_head= (plocal_ipc_data_head_t)local_ipc_data_output_buffer;
        plocal_ipc_data_head->media_type = MEDIA_TYPE_MP3;
        plocal_ipc_data_head->current_sequence = s_u16_current_sequence;
        plocal_ipc_data_head->current_timestamp = current_timestamp;

        plocal_ipc_data_head->local_ipc_data_size = max_payload_bytes*payload_count;
        ret = fread((unsigned char*)&local_ipc_data_output_buffer[LOCAL_IPC_DATA_HEAD_SIZE], 1,plocal_ipc_data_head->local_ipc_data_size, fp);
        if(plocal_ipc_data_head->media_type != s_u16_media_type)
        {
            local_ipc_data_shm_ioctl(&shm,LOCAL_IPC_DATA_SHM_CLEAR_BUF_E,NULL);
        }

#else
        ret = fread(local_ipc_data_output_buffer, 1,max_payload_bytes*payload_count, fp);
#endif
        printf("ret = %ld\n", ret);

        if(ret <= 0)
        {
            break;
        }

        if(s_u32_frame_count == 1000)
        {
            unsigned int frame_rate =  s_u32_frame_count /(current_timestamp - s_u64_last_timestamp) * 1000000;
            printf("frame_rate:%d\n",frame_rate);
            s_u32_frame_count = 0;
            s_u64_last_timestamp = current_timestamp;
        }
        int res=0;
        res = local_ipc_data_shm_write(&shm,local_ipc_data_output_buffer,payload_count);
        if(res != payload_count)
        {
            printf("write error %d\r\n",res);
        }

#if IS_ADD_LOCAL_IPC_DATA_HEAD
        s_u16_media_type = plocal_ipc_data_head->media_type;
        s_u16_current_sequence++;
#endif

        usleep(1000000);//10ms
    } while(ret);

    fclose(fp);
    fp = NULL;
#endif
    local_ipc_data_shm_close(&shm);
    if(local_ipc_data_output_buffer)
    {
        free(local_ipc_data_output_buffer);
        local_ipc_data_output_buffer = NULL;
    }
    return 0;
}
