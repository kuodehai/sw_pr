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
#define READ_FRAME_COUNT  3

int main(int argc, char **argv) // map a normal file as shared mem:
{
    unsigned char *p_mmap;
    init_mmap(0, MEDIA_TYPE_OPUS, "./test", &p_mmap);
    unsigned char read_data_buff[MAX_DATA_BLOCK_SIZE*READ_FRAME_COUNT] = {0};
    while(1)
    {

        read_mmap(p_mmap, READ_FRAME_COUNT, read_data_buff);
        sleep(1);
    }
    destory_mmap(p_mmap);
    return 0;
}
