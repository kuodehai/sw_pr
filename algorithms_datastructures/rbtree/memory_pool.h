#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
/*
¸÷ÖÖ³Ø---ÄÚ´æ³ØµÄ¸ßĞ§ÊµÏÖ£¨CÓïÑÔ£©https://blog.csdn.net/forever5312/article/details/78029672
LinuxÊ¹ÓÃÄÚ´æÓ³ÉäÎÄ¼ş×öÄÚ´æ³Ø  https://blog.csdn.net/cnsword/article/details/7327676
¶ş²æÊ÷±éÀú·Çµİ¹éËã·¨¡ª¡ªÏÈĞò±éÀú https://www.cnblogs.com/greedyco/p/7182555.html
 ¶ş²æÊ÷±éÀú·Çµİ¹éËã·¨¡ª¡ªÖĞĞò±éÀú  https://www.cnblogs.com/greedyco/p/7182768.html
 ¶ş²æÊ÷±éÀú·Çµİ¹éËã·¨¡ª¡ªºóĞò±éÀúhttps://www.cnblogs.com/greedyco/p/7187408.html
 CÓïÑÔÃèÊöÁ´±íµÄÊµÏÖ¼°²Ù×÷ https://www.cnblogs.com/maluning/p/7966875.html
 Ê¹ÓÃÄÚ´æÎÄ¼şÓ³ÉäÊµÏÖ¹²ÏíÄÚ´æ https://blog.csdn.net/nb_vol_1/article/details/51916563

 mmapÊÇlinuxÄÚ´æÓ³ÉäÎÄ¼ş£¬ÊÇ½«ÎÄ¼şÓ³Éä³ÉÎªÄÚ´æµØÖ·¿Õ¼äµÄÒ»ÖÖ·½Ê½£¬ÆäÊµ£¬·½·¨ºÜ¼òµ¥¡£
memfd = open(MEMFILE, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
memd = mmap(NULL, (sizeof(Type)) * size, PROT_WRITE | PROT_READ,
                                    MAP_SHARED, memfd, 0);
                                    //´ó¼ÒÍêÈ«¿ÉÒÔÖ»ÔÚÓ³ÉäÎÄ¼şÖĞ±£´æÊı¾İ£¬Í¨¹ı²»Í¬½ø³ÌÖĞÏàÍ¬µÄÄÚ´æ½á¹¹À´·ÃÎÊ¡£



¸÷ÖÖ³Ø---ÄÚ´æ³ØµÄ¸ßĞ§ÊµÏÖ£¨CÓïÑÔ£ ÒÅÁôÎÊÌâ

1.initÉúĞ§ºó£¬ÄÚ´æ³Ø½á¹¹¹Ì»°£¬ÎŞ·¨¶¯Ì¬À©Õ¹£¬Õâ²¿·ÖºóĞø¿ÉÒÔÌí¼Ó£¬ÓÈÆäÊÇ´øÓĞ×Ô¶¯Ñ§Ï°¹¦ÄÜ£¬ĞèÒªµÄÄÚ´æ´óĞ¡¶àÉú³É£¬²»ĞèÒªµÄ×Ô¶¯¼õÉÙ¡£

2.´Ó´úÂëÉÏ¿´£¬usedÁ´±íÆäÊµÃ»Ê²Ã´ÂÑÓÃ£¬µ±Ê±Éè¼ÆµÄÖ÷ÒªÄ¿µÄÊÇ¹ÜÀíËùÓĞÄÚ´æ½Úµã£¬ËùÓĞÄÚ´æ¼´Ê¹ÓÃ»§²»¹é»¹£¬Ò²¿ÉÒÔ×öÒ»¸ö×Ô¶¯»ØÊÕ»úÖÆ£¬»ØÊÕÄÚ´æ¡£µ«ÊÇÕâ²¿·ÖÃ»×ö¡£¼ÈÈ»Ã»×ö£¬ËùÒÔusedÄ¿Ç°ÊÇ¿ÉÒÔÈ¥³ıµÄ£¬ÒâË¼¾ÍÊÇÓÃ»§ÉêÇëÄÚ´æ£¬°ÑÕâ¸ö½ÚµãÖ±½Ó¸øÓÃ»§£¬ÄÚ´æ³Ø²»ÔÙ¹ÜÀí£¬Èç¹ûÓÃ»§²»¹é»¹£¬ÄÇ¾ÍÊÇÄÚ´æĞ¹Â¶ÁË¡£¡£¡£¡£ÕâÑù×öÓĞºÃ´¦£¬¸ßĞ§¡£

3.ÄÚ´æÊ¹ÓÃÂÊ´òÓ¡º¯Êı¿ÉÒÔÔö¼ÓÒ»Ğ©Í³¼Æ¹¦ÄÜ£¬±ÈÈç ÓÃ»§ÉêÇëÄÚ´æ²»ÊÇÔÚ³ØÖĞ·ÖÅäµÄ£¬×îºÃÒ²¼ÇÂ¼ÏÂÀ´£¬ÕâÑùÎªĞŞ¸Äinit²ÎÊıÌá¹©ÒÀ¾İ¡£

Ò»°ãÖ±½ÓÊ¹ÓÃÒÑÓĞ¹Ì¶¨ÄÚ´æ³Ø


todo: ºìºÚÊ÷ÄÚ´æË÷Òı£¬Ê¹ÓÃÄÚ´æ³ØÀ´´æ·ÅÁ½ÀàÊı¾İ£¬Ò»ÀàÊÇ´æ·ÅºìºÚÊ÷½ÚµãµÄÄÚ´æ³Ø£¬Ò»ÀàÊÇ´æ·Å¼üÖµ½ÚµãµÄÄÚ´æ³Ø¡£
Ê¹ÓÃºìºÚÊ÷Î¬»¤ÄÚ´æ¿é£ºÒ»¸öÒÑ¾­Ê¹ÓÃÄÚ´æ¿éºìºÚÊ÷£¬ÁíÒ»¸ö¿ÕÏĞÄÚ´æ¿éºìºÚÊ÷¡£

³õÊ¼Ê±ÉêÇëÒ»¶¨´óĞ¡µÄÄÚÈİ£¬¸³¸ø×Ô¶¨Òå½Úµã²åÈë¿ÕÏĞÄÚ´æ¿éºìºÚÊ÷
×Ô¶¨Òå½Úµã
struct my_node {
	struct rb_node rb_node;    // ºìºÚÊ÷½Úµã
	Type key;                // ¼üÖµ  ÄÚ´æ¿é´óĞ¡
	//void* memoryblock;;						 // ... ÓÃ»§×Ô¶¨ÒåµÄÊı¾İ  Ö¸Ïò¶ÔÓ¦ÄÚ´æ¿éµØÖ·Ê×µØÖ·(¶¯Ì¬»ñÈ¡£¬Èç¹û¿ÕÏĞÄÚ´æ¿éÒÑÊ¹ÓÃÍê£¬
									  ÉêÇëĞÂµÄÄÚ´æ¿é·ÅÈë¿ÕÏĞÄÚ´æ¿éºìºÚÊ÷£¬ÉêÇëµÄ×ÜÄÚ´æÒªÓĞÏŞÖÆ£¬Ê¹ÓÃµÄÄÚ´æ¿é·ÅÈëÒÑ¾­Ê¹ÓÃÄÚ´æ¿éºìºÚÊ÷£©
};
*/

typedef struct ListNode {
    void*  my_node  ;        //    Êı¾İÓò£¬´æ·ÅÊı¾İ
    struct ListNode *  Next;        //    Ö¸ÏòÏÂÒ»¸öÁ´±í½Úµã
} Node, *PNode;

//     ¶¨ÒåÁ´±í²åÈëº¯Êı
void InsertList(PNode List,void*  my_node);
//¶¨ÒåÉ¾³ıÕû¸öÁ´±íº¯Êı
void DeleteTheList(PNode List);


#define LENGTH(a) ( (sizeof(a)) / (sizeof(a[0])) )
#define  MAX_COLUME_NUM 10
int  get_memblock_head_len();
int      buffer_pool_init(unsigned int colume_no, unsigned int block_len[], unsigned int block_count[]);
void * buffer_malloc(unsigned int memblock_size);
int     buffer_free(void * memblock);
int     buffer_pool_destory(void);

int  free_buffer_pool_print(void);
int  used_buffer_pool_print(void);
int   buffer_runtime_print(void);


#endif
