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
#include "sharemem_mmap.h"

int main(int argc, char **argv)
{
    unsigned char *p_mmap;
    init_mmap(1, MEDIA_TYPE_OPUS, "./test", &p_mmap);

    long i=0;
    char in_data_buff[MAX_DATA_BLOCK_SIZE];
    int in_data_len = MAX_DATA_BLOCK_SIZE;
#if 1
    int   j ='a';
    while(1)
    {

        if(j > 'z')
        {
            j= 'a';
        }
        memset(in_data_buff, j, in_data_len);

        ATTRIBUTE_BUFFER_BLOCK_T attribute_buffer_block_t;
        memset(&attribute_buffer_block_t,0,sizeof(ATTRIBUTE_BUFFER_BLOCK_T));
        attribute_buffer_block_t.media_type = MEDIA_TYPE_OPUS;
        attribute_buffer_block_t.current_sequence = i;
        attribute_buffer_block_t.current_timestamp = gettimeofdayUsec();
        attribute_buffer_block_t.ssrc = 0;
        attribute_buffer_block_t.payload_size = in_data_len;
        write_mmap(p_mmap,in_data_buff, attribute_buffer_block_t);
        sleep(1);
        i++;
        j++;
    }
#else



    const char* filename = "./0-8000Hz.wav";
    FILE *fp = NULL;
    int write_length = 0;
    fp = fopen(filename, "ab+");
    if(fp == NULL)
    {
        return 0;
    }

    size_t  ret = 0;
    do
    {
        ret = fread(in_data_buff, 1,in_data_len, fp);
        printf("ret = %ld\n", ret);

        ATTRIBUTE_BUFFER_BLOCK_T attribute_buffer_block_t;
        memset(&attribute_buffer_block_t,0,sizeof(ATTRIBUTE_BUFFER_BLOCK_T));
        attribute_buffer_block_t.media_type = MEDIA_TYPE_OPUS;
        attribute_buffer_block_t.current_sequence = i;
        attribute_buffer_block_t.current_timestamp = gettimeofdayUsec();
        attribute_buffer_block_t.ssrc = 0;
        attribute_buffer_block_t.payload_size = ret;
        if(ret <= 0)
        {
            break;
        }
        write_mmap(p_mmap,in_data_buff, attribute_buffer_block_t);
        usleep(10000);//10ms
        i++;
        write_length+= ret;
        fseek(fp,write_length,SEEK_CUR);



    } while(ret);

    fclose(fp);
    fp = NULL;
    return 0;




#endif
    destory_mmap(p_mmap);
    return 0;
}
