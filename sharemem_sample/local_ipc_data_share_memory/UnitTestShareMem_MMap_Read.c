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

int main(int argc, char **argv) // map a normal file as shared mem:
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
    int payload_count = 3; //media data   read sharemem  frame num

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

#if 1 //recv data  to save file
    const char* filename = "./music_mp3_128_save.mp3"; //frame_size 44100*2  //音频和图片等需要要 获取完整帧数据后 按帧长的整数倍写文件	才可以打开播放显示
    FILE *fp = NULL;
    fp = fopen(filename, "ab+");
    if(fp == NULL)
    {
        return 0;
    }
    size_t  ret = 0;
#endif
    while(1)
    {

        ret = local_ipc_data_shm_read(&shm, local_ipc_data_output_buffer,payload_count);
        ///////////////////////////////
        printf("flocal_ipc_data_shm_read ret = %ld  \n", ret);
        ///////////////////////////////////
        if(ret == payload_count)
        {
#if 1 //recv data  to save file	
            //fwrite(buf, sizeof(buf的数据类型), 帧写入长度, pcm_file);
#if IS_ADD_LOCAL_IPC_DATA_HEAD
            static unsigned short s_u16_current_sequence = 0;
            for(int i=0; i<payload_count; i++)
            {
                plocal_ipc_data_head_t plocal_ipc_data_head= (plocal_ipc_data_head_t)(local_ipc_data_output_buffer +i * ( LOCAL_IPC_DATA_HEAD_SIZE + max_payload_bytes));
                //plocal_ipc_data_head->current_sequence = current_sequence;
                //plocal_ipc_data_head->current_timestamp = gettimeofdayUsec();
                switch(plocal_ipc_data_head->media_type)
                {
                case MEDIA_TYPE_PCM:
                    break;
                case MEDIA_TYPE_OPUS:
                    break;
                case MEDIA_TYPE_G711:
                    break;
                case MEDIA_TYPE_G722:
                    break;
                case MEDIA_TYPE_MP3:
                    break;
                case MEDIA_TYPE_H264:
                    break;
                case MEDIA_TYPE_H265:
                    break;

                }

                ret = fwrite( local_ipc_data_output_buffer +i * ( LOCAL_IPC_DATA_HEAD_SIZE + max_payload_bytes)+ LOCAL_IPC_DATA_HEAD_SIZE,
                              1,  max_payload_bytes, fp);
                if(ret !=  max_payload_bytes )
                {
                    printf("fwrite  fail!!!  ret = %ld\n", ret);

                }
                if(plocal_ipc_data_head->current_sequence != s_u16_current_sequence)
                {
                    printf("local ipc data shm lose frame plocal_ipc_data_head->current_sequence:%d != s_u16_current_sequence:%d\n",
                           plocal_ipc_data_head->current_sequence,s_u16_current_sequence);
                }
                s_u16_current_sequence = plocal_ipc_data_head->current_sequence + 1;

            }

#else
            ret = fwrite((unsigned char*)&local_ipc_data_output_buffer[0], 1,payload_count * max_payload_bytes, fp);
            if(ret != (payload_count * max_payload_bytes))
            {
                printf("fwrite  fail!!!  ret = %ld\n", ret);

            }
#endif


#endif
        }
        else
        {
            //printf("local_ipc_data_shm_read  fail!!!  ret = %ld\n", ret);
        }


        usleep(1000000);//10ms
    }

    fclose(fp);
    fp = NULL;
    local_ipc_data_shm_close(&shm);
    if(local_ipc_data_output_buffer)
    {
        free(local_ipc_data_output_buffer);
        local_ipc_data_output_buffer = NULL;
    }

    return 0;
}
