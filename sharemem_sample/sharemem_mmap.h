/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */


#ifndef SHARE_MEM_MMAP_H
#define SHARE_MEM_MMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLOCK_COUNT 256
#define MAX_DATA_BLOCK_SIZE 76
#define MIN_PAYLOAD_COUNT 6

typedef enum {
    MEDIA_TYPE_OPUS = 1,
    MEDIA_TYPE_G711,
    MEDIA_TYPE_G722
}
MEDIA_TYPE_E;


typedef struct {
    unsigned int index; //0- (current_sequence%jiter_capacity)
    unsigned short media_type;
    unsigned short current_sequence;

    unsigned long current_timestamp;
    unsigned int ssrc;
    unsigned int payload_size;

    // unsigned char *payload_buffer;
} ATTRIBUTE_BUFFER_BLOCK_T;


unsigned long gettimeofdayUsec(void);

int init_mmap(const char is_write, const unsigned short media_type,
              const char *name_file, unsigned char **p_mmap);
int destory_mmap(unsigned char *p_mmap) ;

int write_mmap(unsigned char *p_mmap, const unsigned char *buffer,
               ATTRIBUTE_BUFFER_BLOCK_T attribute_buffer_block_t);

int read_mmap(const unsigned char *p_mmap, const unsigned int read_frame_count,
              unsigned char *read_data_buff);

#ifdef __cplusplus
}
#endif


#endif // SHARE_MEM_MMAP_H

