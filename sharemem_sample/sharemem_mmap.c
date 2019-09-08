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


typedef enum {
    WDP_LOG_RUN_ALL =   0x1 << 0, //1
    WDP_LOG_RUN_TRACE = 0x1 << 1, //2
    WDP_LOG_RUN_DEBUG = 0x1 << 2, //4
    WDP_LOG_RUN_INFO =  0x1 << 3, //8
    WDP_LOG_RUN_WARN =  0x1 << 4, //16
    WDP_LOG_RUN_ERROR = 0x1 << 5, //32
    WDP_LOG_RUN_EFATAL= 0x1 << 6, //64
    WDP_LOG_RUN_OFF =   0x1 << 7 //128
} WDP_LOG_RUN_LEVEL_E;

static int g_current_wdp_log_level = WDP_LOG_RUN_DEBUG | WDP_LOG_RUN_INFO |  WDP_LOG_RUN_WARN | WDP_LOG_RUN_ERROR | WDP_LOG_RUN_EFATAL ;

#define WDP_LOG_RUN(current_log_level,format,...)  do{\
    if(g_current_wdp_log_level & current_log_level ){\
        printf("log level %d: [%d-%s]  " format "\n\r",  current_log_level,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
      }\
    }while(0)


typedef struct {

    unsigned long max_data_size;  //jiter总长度
    unsigned int attribute_buffer_size; //预留保存的所有载荷头信息
    unsigned int per_payload_buffer_size;//每个载荷数据长度

    //对于JB来说，需要知道当前的运行状态以及一些统计信息等。如果这些信息正常，就说明问题很大可能不是由JB引起的，不正常则说明有很大的可能性。
    unsigned short jiter_status; // JB当前运行状态：prefetching / processing
    unsigned short jiter_cached_paload_count; // JB里有多少个缓存的包
    unsigned short jiter_get_offset;          //从JB中取帧的head的位置
    unsigned short jiter_capacity;            //缓冲区的capacity

    unsigned short net_lost_package_count; //网络丢包的个数
    unsigned short late_arrival_discard_package_count; //由于来的太迟而被主动丢弃的包的个数

    unsigned short
    discard_exists_package_count; //由于JB里已有这个包而被主动丢弃的包的个数
    unsigned short
    jiter_enter_prefetching_state_count; //进prefetching状态的次数（除了第一次）
    //

    unsigned short media_type; // key 媒体类型
    unsigned short last_got_senq; //：设上一个已经取出的包的sequence number为 last_got_senq

    unsigned long last_got_timestamp;//设上一个已经取出的包的timestamp 为 last_got_timestamp

    unsigned short last_put_position; //设上一个已放好的包的position为 last_put_position
    unsigned short last_put_senq; //设上一个已放好的包的sequence number为 last_put_senq
    unsigned long last_put_timestamp; //设上一个已放好的包的timestamp 为 last_put_timestamp

} jiter_buffer_head_t;




typedef struct {
    unsigned char data[MAX_DATA_BLOCK_SIZE];
} PAYLOAD_BUFFER_BLOCK_T;

typedef enum {
    JITER_STATUS_PREFETCHING = 1, //预存取   初始化时把状态置成prefetching.
    //当在JB中的语音包个数达到指定的值时便把状态切到processing
    JITER_STATUS_PROCESSINIG //处理中  只有在processing时才能从JB中取到语音帧.
    //如果从JB里取不到语音帧了，它将又回到prefetching
} JITER_STATUS_E;

static unsigned int get_packet_time(const unsigned short media_type) {
    unsigned int packet_time = 0;
    switch (media_type) // packet_time opus:10ms  g711/g722 20ms
    {
    case MEDIA_TYPE_OPUS:
        packet_time = 10;
        break;

    default:
        break;
    }
    return packet_time;
}

unsigned long gettimeofdayUsec(void)
{
    struct timeval stamp;
    gettimeofday(&stamp, NULL);
    return (stamp.tv_sec*1000000 + stamp.tv_usec);
}


static int resetJiter(unsigned char *p_mmap, const unsigned short media_type) {

    jiter_buffer_head_t *pjiter_buffer_head = (jiter_buffer_head_t *)p_mmap;
    if (p_mmap == NULL || pjiter_buffer_head == NULL) {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"p_mmap:%p  pjiter_buffer_head:%p retun -1\n", p_mmap,
                    pjiter_buffer_head);
        return -1;
    }

    unsigned int packet_time = get_packet_time(media_type);
    unsigned int capacity = MAX_BLOCK_COUNT * 10 / packet_time;
    unsigned int attribute_buffer_size =
        sizeof(ATTRIBUTE_BUFFER_BLOCK_T) * capacity;
    unsigned int per_payload_buffer_block_count = packet_time / 10;
    unsigned int per_payload_buffer_size =
        sizeof(PAYLOAD_BUFFER_BLOCK_T) *
        per_payload_buffer_block_count; // attribute_buffer_block.payload_size
    unsigned int payload_buffer_size = per_payload_buffer_size * capacity;

    unsigned int max_data_size =
        sizeof(jiter_buffer_head_t) + attribute_buffer_size + payload_buffer_size;

    memset(p_mmap, 0, max_data_size);
    pjiter_buffer_head->media_type = media_type;
    pjiter_buffer_head->jiter_status = JITER_STATUS_PREFETCHING;
    pjiter_buffer_head->jiter_capacity = capacity;

    pjiter_buffer_head->max_data_size = max_data_size;
    pjiter_buffer_head->attribute_buffer_size = attribute_buffer_size;
    pjiter_buffer_head->per_payload_buffer_size = per_payload_buffer_size;
    return 0;
}

int init_mmap(const char is_write, const unsigned short media_type,
              const char *name_file, unsigned char **p_mmap) {

    unsigned int packet_time = get_packet_time(media_type);
    unsigned int capacity = MAX_BLOCK_COUNT * 10 / packet_time;
    unsigned int attribute_buffer_size =
        sizeof(ATTRIBUTE_BUFFER_BLOCK_T) * capacity;
    unsigned int per_payload_buffer_block_count = packet_time / 10;
    unsigned int per_payload_buffer_size =
        sizeof(PAYLOAD_BUFFER_BLOCK_T) *
        per_payload_buffer_block_count; // attribute_buffer_block.payload_size
    unsigned int payload_buffer_size = per_payload_buffer_size * capacity;

    unsigned int max_data_size =
        sizeof(jiter_buffer_head_t) + attribute_buffer_size + payload_buffer_size;

    int pagesize = sysconf(_SC_PAGESIZE);
    int page_count = max_data_size / pagesize;
    page_count = max_data_size % pagesize > 0 ? (page_count + 1) : page_count;
    WDP_LOG_RUN(WDP_LOG_RUN_INFO,"max_data_size:%d  capacity:%d pagesize is %d  page_count:%d", max_data_size, capacity,
                pagesize, page_count);
    int fd = -1;
    if (is_write) {
        fd = open(name_file, O_CREAT | O_RDWR | O_TRUNC, 00777);
    } else {
        fd = open(name_file, O_CREAT | O_RDWR, 00777);
    }

    struct stat sb;
    /* 获取文件的属性 */
    if ((fstat(fd, &sb)) == -1) {
        perror("fstat");
    }
    WDP_LOG_RUN(WDP_LOG_RUN_INFO,"st_size:%ld",sb.st_size);
    if (sb.st_size == 0) {
        lseek(fd, pagesize * page_count - 1, SEEK_SET);
        write(fd, "\0", pagesize * page_count);
    }

    *p_mmap = (unsigned char *)mmap(
                  NULL, max_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                  0); //此处offset = 0编译成版本1；offset = pdata_lensize编译成版本2
    ////注意：文件被映射部分而不是整个文件决定了进程能够访问的空间大小，另外，如果指定文件的偏移部分，一定要注意为页面大小的整数倍。下面是对进程映射地址空间的访问范例：

    if (*p_mmap == MAP_FAILED || *p_mmap == NULL) {
        perror("MAP failed");
        close(fd);
        return -1;
    }

    if (sb.st_size == 0) {

        struct stat sb;
        /* 获取文件的属性 */
        if ((fstat(fd, &sb)) == -1) {
            perror("fstat");
        }

        memset(*p_mmap, 0, max_data_size);
        jiter_buffer_head_t *pjiter_buffer_head = (jiter_buffer_head_t *)(*p_mmap);
        // memset(pjiter_buffer_head,0,sizeof(jiter_buffer_head_t));
        pjiter_buffer_head->media_type = media_type;
        pjiter_buffer_head->jiter_status = JITER_STATUS_PREFETCHING;
        pjiter_buffer_head->jiter_capacity = capacity;
        pjiter_buffer_head->max_data_size = max_data_size;
        pjiter_buffer_head->attribute_buffer_size = attribute_buffer_size;
        pjiter_buffer_head->per_payload_buffer_size = per_payload_buffer_size;

        WDP_LOG_RUN(WDP_LOG_RUN_INFO,"st_size:%ld media_type:%d jiter_status:%d  jiter_capacity:%d attribute_buffer_size:%d  per_payload_buffer_size:%d",
                    sb.st_size,
                    pjiter_buffer_head->media_type, pjiter_buffer_head->jiter_status,   pjiter_buffer_head->jiter_capacity ,
                    pjiter_buffer_head->attribute_buffer_size, pjiter_buffer_head->per_payload_buffer_size);
    }
    close(fd);
    return 0;
}

int destory_mmap(unsigned char *p_mmap) {
    jiter_buffer_head_t *pjiter_buffer_head = (jiter_buffer_head_t *)p_mmap;
    if (p_mmap == NULL || pjiter_buffer_head == NULL) {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"p_mmap:%p  pjiter_buffer_head:%p retun -1\n", p_mmap,
                    pjiter_buffer_head);
        return -1;
    }
    munmap(p_mmap, pjiter_buffer_head->max_data_size);
    return 0;
}

int write_mmap(unsigned char *p_mmap, const unsigned char *buffer,
               ATTRIBUTE_BUFFER_BLOCK_T attribute_buffer_block_t) {

    if (p_mmap == NULL ) {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"p_mmap:%p   retun -1\n",p_mmap);
        return -1;
    }

    jiter_buffer_head_t *pjiter_buffer_head = (jiter_buffer_head_t *)p_mmap;

    //在以下情况下需要reset JB，让JB在初始状态下开始运行。
    if (attribute_buffer_block_t.media_type != pjiter_buffer_head->media_type) {
        //当收到的语音包的媒体类型（G711/G722/G729，不包括SID/RFC2833等）变了，就认为来了新的stream，需要reset
        // JB。
        resetJiter(p_mmap,
                   attribute_buffer_block_t.media_type); // note: sync  read_mmap
    }
    //当收到的语音包的SSRC变了，就认为来了新的stream，需要reset JB。
    //当收到的语音包的packet time变了，就认为来了新的stream，需要reset JB。

    unsigned short delta_senq = 0;
    // delta_senq可以根据下面的逻辑关系得到。
    if (attribute_buffer_block_t.current_sequence >=
            pjiter_buffer_head->last_got_senq) {
        if (attribute_buffer_block_t.current_timestamp >=
                pjiter_buffer_head->last_got_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence -
                         pjiter_buffer_head->last_got_senq;
        } else if (attribute_buffer_block_t.current_timestamp <
                   pjiter_buffer_head->last_got_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence - 65536 -
                         pjiter_buffer_head->last_got_senq;
        }
    } else if (attribute_buffer_block_t.current_sequence <
               pjiter_buffer_head->last_got_senq) {
        if (attribute_buffer_block_t.current_timestamp >=
                pjiter_buffer_head->last_got_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence + 65536 -
                         pjiter_buffer_head->last_got_senq;
        } else if (attribute_buffer_block_t.current_timestamp <
                   pjiter_buffer_head->last_got_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence -
                         pjiter_buffer_head->last_got_senq;
        }
    }
    if (pjiter_buffer_head->jiter_cached_paload_count != 0 &&
            delta_senq <1) //如果delta_senq小于1，就可以认为这个包来的太迟，就要主动丢弃掉。由于我们的buffer足够大（256个block），如果包早到了也会被放到对应的position上，不会把相应位置上的还没取走的覆盖掉。
    {
        pjiter_buffer_head->late_arrival_discard_package_count++;
        WDP_LOG_RUN(WDP_LOG_RUN_INFO,"late arrival discard package current_sequence:%d  late_arrival_discard_package_count:%d retun -1\n",attribute_buffer_block_t.current_sequence, pjiter_buffer_head->late_arrival_discard_package_count);
        return -1;
    }

    unsigned int cur_position = 0;
    // cur_position 可以根据下面的逻辑关系得到。 接下来看怎么把包放到正确的位置上
    // 它的位置（position，范围是0 ~ capacity-1）

    if (attribute_buffer_block_t.current_sequence >=
            pjiter_buffer_head->last_put_senq) {
        if (attribute_buffer_block_t.current_timestamp >=
                pjiter_buffer_head->last_put_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence -
                         pjiter_buffer_head->last_put_senq;
        } else if (attribute_buffer_block_t.current_timestamp <
                   pjiter_buffer_head->last_put_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence - 65536 -
                         pjiter_buffer_head->last_put_senq;
        }
    } else if (attribute_buffer_block_t.current_sequence <
               pjiter_buffer_head->last_put_senq) {
        if (attribute_buffer_block_t.current_timestamp >=
                pjiter_buffer_head->last_put_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence + 65536 -
                         pjiter_buffer_head->last_put_senq;
        } else if (attribute_buffer_block_t.current_timestamp <
                   pjiter_buffer_head->last_put_timestamp) {
            delta_senq = attribute_buffer_block_t.current_sequence -
                         pjiter_buffer_head->last_put_senq;
        }
    }

    unsigned short last_put_position  = pjiter_buffer_head->last_put_position;
    unsigned int jiter_capacity =  pjiter_buffer_head->jiter_capacity;
    cur_position = last_put_position + delta_senq +jiter_capacity ;
    if(cur_position >= jiter_capacity)
        cur_position = cur_position - jiter_capacity ;
    //得到了当前包的position后就可以把包头里的timestamp等放到相应的attribute
    // buffer
    // block里了，payload根据算好的占几个block放到相应的那几个block上（有可能填不满block，不过没关系，取payload时是根据index取的）。
    ATTRIBUTE_BUFFER_BLOCK_T *pAttribute_buffer_block_t =
        (ATTRIBUTE_BUFFER_BLOCK_T *)(p_mmap + sizeof(jiter_buffer_head_t) +
                                     sizeof(ATTRIBUTE_BUFFER_BLOCK_T) *
                                     cur_position);
    if (pAttribute_buffer_block_t == NULL)
    {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"pAttribute_buffer_block_t:%p retun -1\n",pAttribute_buffer_block_t);
        return -1;
    }

    if (pjiter_buffer_head->jiter_cached_paload_count != 0 &&
            attribute_buffer_block_t.current_sequence ==  pAttribute_buffer_block_t->current_sequence) {
        //如果放进对应block时发现里面已经有包了并且sequence
        // number一样，说明这个包是重复包，就要把这个包主动丢弃掉。
        pjiter_buffer_head->discard_exists_package_count++;
        WDP_LOG_RUN(WDP_LOG_RUN_INFO,"discard_exists_package  current_sequence:%d  discard_exists_package_count:%d retun -1\n",attribute_buffer_block_t.current_sequence,pjiter_buffer_head->discard_exists_package_count);
        return -1;
    }

    attribute_buffer_block_t.index = attribute_buffer_block_t.current_sequence %   pjiter_buffer_head->jiter_capacity;

    memcpy(pAttribute_buffer_block_t, &attribute_buffer_block_t,
           sizeof(ATTRIBUTE_BUFFER_BLOCK_T));
    memcpy(p_mmap + sizeof(jiter_buffer_head_t) +
           pjiter_buffer_head->attribute_buffer_size +
           pjiter_buffer_head->per_payload_buffer_size * cur_position,
           buffer,
           attribute_buffer_block_t.payload_size);

    pjiter_buffer_head->media_type = attribute_buffer_block_t.media_type;
    pjiter_buffer_head->last_put_position = cur_position;
    pjiter_buffer_head->last_put_senq = attribute_buffer_block_t.current_sequence;
    pjiter_buffer_head->last_put_timestamp = attribute_buffer_block_t.current_timestamp;

    pjiter_buffer_head->jiter_cached_paload_count++;
    if (pjiter_buffer_head->jiter_cached_paload_count >=
            MIN_PAYLOAD_COUNT) //当在JB中的语音包个数达到指定的值时便把状态切到
        // JITER_STATUS_PROCESSINIG
    {
        pjiter_buffer_head->jiter_status = JITER_STATUS_PROCESSINIG;
    }
    WDP_LOG_RUN(WDP_LOG_RUN_DEBUG,"media_type:%d  last_put_position:%d last_put_senq:%d  last_put_timestamp:%ld  jiter_cached_paload_count:%d  jiter_status:%d",
                pjiter_buffer_head->media_type,   pjiter_buffer_head->last_put_position, pjiter_buffer_head->last_put_senq,
                pjiter_buffer_head->last_put_timestamp, pjiter_buffer_head->jiter_cached_paload_count, pjiter_buffer_head->jiter_status);
    return 0;
}

int read_mmap(const unsigned char *p_mmap, const unsigned int read_frame_count,
              unsigned char *read_data_buff) {
    //再来看GET操作。每次从JB里不是取一个包，而是取1帧（能编解码的最小单位，通常是10ms，也有例外，比如AMR-WB是20ms），
    //这主要是因为播放loop是10ms一次（每次都是取一帧语音数据播放）。
    //取时总是从head上取，开始时head为第一个放进JB的包的position，每取完一个包（几帧）后head就会向后移一个位置。
    //如果到某个位置时它的block里没有包，就说明这个包丢了，这时取出的就是payload大小就是0，告诉后续的decoder要做PLC。不同类型的包取法不一样，
    jiter_buffer_head_t *pjiter_buffer_head = (jiter_buffer_head_t *)p_mmap;
    if (p_mmap == NULL || pjiter_buffer_head == NULL) {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"p_mmap:%p  pjiter_buffer_head:%p retun -1\n", p_mmap,
                    pjiter_buffer_head);
        return -1;
    }

    WDP_LOG_RUN(WDP_LOG_RUN_DEBUG,"jiter_status:%d jiter_cached_paload_count:%d jiter_get_offset:%d  jiter_capacity:%d  net_lost_package_count:%d  late_arrival_discard_package_count:%d discard_exists_package_count:%d  jiter_enter_prefetching_state_count:%d \n",
                pjiter_buffer_head->jiter_status, pjiter_buffer_head->jiter_cached_paload_count,
                pjiter_buffer_head->jiter_get_offset, pjiter_buffer_head->jiter_capacity,
                pjiter_buffer_head->net_lost_package_count, pjiter_buffer_head->late_arrival_discard_package_count,
                pjiter_buffer_head->discard_exists_package_count,pjiter_buffer_head->jiter_enter_prefetching_state_count);

    if(read_frame_count < 1 &&
            read_frame_count > pjiter_buffer_head->jiter_capacity )
    {
        WDP_LOG_RUN(WDP_LOG_RUN_WARN,"read_frame_count:%d < pjiter_buffer_head->jiter_capacity:%d",
                    read_frame_count,pjiter_buffer_head->jiter_capacity);
        return -1;
    }

    if(pjiter_buffer_head->jiter_cached_paload_count <= read_frame_count)
    {
        WDP_LOG_RUN(WDP_LOG_RUN_TRACE,"pjiter_buffer_head->jiter_cached_paload_count:%d <= read_frame_count:%d",
                    pjiter_buffer_head->jiter_cached_paload_count,read_frame_count);
        return -1;
    }



    if (pjiter_buffer_head->jiter_status == JITER_STATUS_PROCESSINIG)
    {
        // todo:
        pjiter_buffer_head->jiter_get_offset =
            pjiter_buffer_head->jiter_get_offset %
            pjiter_buffer_head->jiter_capacity;


        ATTRIBUTE_BUFFER_BLOCK_T *pAttribute_buffer_block_t =
            (ATTRIBUTE_BUFFER_BLOCK_T *)(p_mmap + sizeof(jiter_buffer_head_t) +
                                         sizeof(ATTRIBUTE_BUFFER_BLOCK_T) *
                                         pjiter_buffer_head->jiter_get_offset);
        if (pAttribute_buffer_block_t == NULL) {
            WDP_LOG_RUN(WDP_LOG_RUN_WARN,"pAttribute_buffer_block_t:%p  retun -1\n", pAttribute_buffer_block_t);
            return -1;
        }

        const unsigned char *  copybuffer = (p_mmap + sizeof(jiter_buffer_head_t) + pjiter_buffer_head->attribute_buffer_size) +
                                            MAX_DATA_BLOCK_SIZE * pAttribute_buffer_block_t->index;

        int remain_offset = pjiter_buffer_head->jiter_capacity -   pjiter_buffer_head->jiter_get_offset;
        if( remain_offset >= read_frame_count)
        {
            memcpy(read_data_buff, copybuffer,read_frame_count*MAX_DATA_BLOCK_SIZE);
        }
        else
        {
            int remain_size = remain_offset*MAX_DATA_BLOCK_SIZE;
            memcpy(read_data_buff, copybuffer,remain_size);
            memcpy(read_data_buff +remain_size,
                   (p_mmap + sizeof(jiter_buffer_head_t) + pjiter_buffer_head->attribute_buffer_size) ,
                   (read_frame_count-remain_offset)*MAX_DATA_BLOCK_SIZE);

        }



        // *read_data_len = pAttribute_buffer_block_t->payload_size;

        WDP_LOG_RUN(WDP_LOG_RUN_INFO,"current_sequence:%d  current_timestamp:%ld read_data_buff:%s \n ",
                    pAttribute_buffer_block_t->current_sequence,  pAttribute_buffer_block_t->current_timestamp,read_data_buff);
        pjiter_buffer_head->last_got_senq =
            pAttribute_buffer_block_t->current_sequence;
        pjiter_buffer_head->last_got_timestamp =
            pAttribute_buffer_block_t->current_timestamp;

        pjiter_buffer_head->jiter_cached_paload_count-= read_frame_count;
        pjiter_buffer_head->jiter_get_offset+=read_frame_count ;
        if (pjiter_buffer_head->jiter_cached_paload_count <
                MIN_PAYLOAD_COUNT) //当在JB中的语音包个数未达到指定的值时便把状态切到
            // JITER_STATUS_PREFETCHING
        {
            pjiter_buffer_head->jiter_status = JITER_STATUS_PREFETCHING;
            pjiter_buffer_head->jiter_enter_prefetching_state_count++;
        }
    }
}


