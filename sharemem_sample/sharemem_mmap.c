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

    unsigned long max_data_size;  //jiter�ܳ���
    unsigned int attribute_buffer_size; //Ԥ������������غ�ͷ��Ϣ
    unsigned int per_payload_buffer_size;//ÿ���غ����ݳ���

    //����JB��˵����Ҫ֪����ǰ������״̬�Լ�һЩͳ����Ϣ�ȡ������Щ��Ϣ��������˵������ܴ���ܲ�����JB����ģ���������˵���кܴ�Ŀ����ԡ�
    unsigned short jiter_status; // JB��ǰ����״̬��prefetching / processing
    unsigned short jiter_cached_paload_count; // JB���ж��ٸ�����İ�
    unsigned short jiter_get_offset;          //��JB��ȡ֡��head��λ��
    unsigned short jiter_capacity;            //��������capacity

    unsigned short net_lost_package_count; //���綪���ĸ���
    unsigned short late_arrival_discard_package_count; //��������̫�ٶ������������İ��ĸ���

    unsigned short
    discard_exists_package_count; //����JB������������������������İ��ĸ���
    unsigned short
    jiter_enter_prefetching_state_count; //��prefetching״̬�Ĵ��������˵�һ�Σ�
    //

    unsigned short media_type; // key ý������
    unsigned short last_got_senq; //������һ���Ѿ�ȡ���İ���sequence numberΪ last_got_senq

    unsigned long last_got_timestamp;//����һ���Ѿ�ȡ���İ���timestamp Ϊ last_got_timestamp

    unsigned short last_put_position; //����һ���ѷźõİ���positionΪ last_put_position
    unsigned short last_put_senq; //����һ���ѷźõİ���sequence numberΪ last_put_senq
    unsigned long last_put_timestamp; //����һ���ѷźõİ���timestamp Ϊ last_put_timestamp

} jiter_buffer_head_t;




typedef struct {
    unsigned char data[MAX_DATA_BLOCK_SIZE];
} PAYLOAD_BUFFER_BLOCK_T;

typedef enum {
    JITER_STATUS_PREFETCHING = 1, //Ԥ��ȡ   ��ʼ��ʱ��״̬�ó�prefetching.
    //����JB�е������������ﵽָ����ֵʱ���״̬�е�processing
    JITER_STATUS_PROCESSINIG //������  ֻ����processingʱ���ܴ�JB��ȡ������֡.
    //�����JB��ȡ��������֡�ˣ������ֻص�prefetching
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
    /* ��ȡ�ļ������� */
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
                  0); //�˴�offset = 0����ɰ汾1��offset = pdata_lensize����ɰ汾2
    ////ע�⣺�ļ���ӳ�䲿�ֶ����������ļ������˽����ܹ����ʵĿռ��С�����⣬���ָ���ļ���ƫ�Ʋ��֣�һ��Ҫע��Ϊҳ���С���������������ǶԽ���ӳ���ַ�ռ�ķ��ʷ�����

    if (*p_mmap == MAP_FAILED || *p_mmap == NULL) {
        perror("MAP failed");
        close(fd);
        return -1;
    }

    if (sb.st_size == 0) {

        struct stat sb;
        /* ��ȡ�ļ������� */
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

    //�������������Ҫreset JB����JB�ڳ�ʼ״̬�¿�ʼ���С�
    if (attribute_buffer_block_t.media_type != pjiter_buffer_head->media_type) {
        //���յ�����������ý�����ͣ�G711/G722/G729��������SID/RFC2833�ȣ����ˣ�����Ϊ�����µ�stream����Ҫreset
        // JB��
        resetJiter(p_mmap,
                   attribute_buffer_block_t.media_type); // note: sync  read_mmap
    }
    //���յ�����������SSRC���ˣ�����Ϊ�����µ�stream����Ҫreset JB��
    //���յ�����������packet time���ˣ�����Ϊ�����µ�stream����Ҫreset JB��

    unsigned short delta_senq = 0;
    // delta_senq���Ը���������߼���ϵ�õ���
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
            delta_senq <1) //���delta_senqС��1���Ϳ�����Ϊ���������̫�٣���Ҫ�������������������ǵ�buffer�㹻��256��block����������絽��Ҳ�ᱻ�ŵ���Ӧ��position�ϣ��������Ӧλ���ϵĻ�ûȡ�ߵĸ��ǵ���
    {
        pjiter_buffer_head->late_arrival_discard_package_count++;
        WDP_LOG_RUN(WDP_LOG_RUN_INFO,"late arrival discard package current_sequence:%d  late_arrival_discard_package_count:%d retun -1\n",attribute_buffer_block_t.current_sequence, pjiter_buffer_head->late_arrival_discard_package_count);
        return -1;
    }

    unsigned int cur_position = 0;
    // cur_position ���Ը���������߼���ϵ�õ��� ����������ô�Ѱ��ŵ���ȷ��λ����
    // ����λ�ã�position����Χ��0 ~ capacity-1��

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
    //�õ��˵�ǰ����position��Ϳ��԰Ѱ�ͷ���timestamp�ȷŵ���Ӧ��attribute
    // buffer
    // block���ˣ�payload������õ�ռ����block�ŵ���Ӧ���Ǽ���block�ϣ��п������block������û��ϵ��ȡpayloadʱ�Ǹ���indexȡ�ģ���
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
        //����Ž���Ӧblockʱ���������Ѿ��а��˲���sequence
        // numberһ����˵����������ظ�������Ҫ�������������������
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
            MIN_PAYLOAD_COUNT) //����JB�е������������ﵽָ����ֵʱ���״̬�е�
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
    //������GET������ÿ�δ�JB�ﲻ��ȡһ����������ȡ1֡���ܱ�������С��λ��ͨ����10ms��Ҳ�����⣬����AMR-WB��20ms����
    //����Ҫ����Ϊ����loop��10msһ�Σ�ÿ�ζ���ȡһ֡�������ݲ��ţ���
    //ȡʱ���Ǵ�head��ȡ����ʼʱheadΪ��һ���Ž�JB�İ���position��ÿȡ��һ��������֡����head�ͻ������һ��λ�á�
    //�����ĳ��λ��ʱ����block��û�а�����˵����������ˣ���ʱȡ���ľ���payload��С����0�����ߺ�����decoderҪ��PLC����ͬ���͵İ�ȡ����һ����
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
                MIN_PAYLOAD_COUNT) //����JB�е�����������δ�ﵽָ����ֵʱ���״̬�е�
            // JITER_STATUS_PREFETCHING
        {
            pjiter_buffer_head->jiter_status = JITER_STATUS_PREFETCHING;
            pjiter_buffer_head->jiter_enter_prefetching_state_count++;
        }
    }
}


