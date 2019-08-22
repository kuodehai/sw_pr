
#include "Common/Interfaces/memory_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include<time.h>
#include<string.h>


#define LENGTH(a) ( (sizeof(a)) / (sizeof(a[0])) )

int main()
{
    unsigned int a[10] = { 8 ,16 ,32 ,64 ,128 , 256 , 512 , 1024 ,2028 , 4096 };
    //unsigned int b[10] = { 100 ,500 ,1000 ,1500 ,2000 , 3000 , 4000 , 5000 ,4000 , 2000 };
    //unsigned int b[10] = { 1 ,5 ,10 ,15 ,20 , 30 , 40 , 50 ,40 , 20 };
    unsigned int b[10] = { 1 ,1,1 ,1 ,1 , 1 , 1 , 1 ,1 , 1 };
    int i, ilen = LENGTH(a);
    for (i = 0; i<ilen; i++)
        printf("%d ", a[i]);
    printf("\n");
    buffer_pool_init(10, a, b);



    char  * memBlock1 = buffer_malloc(6);
    char  * memBlock2 = buffer_malloc(172);
    char  * memBlock3 = buffer_malloc(186);

    used_buffer_pool_print();

    buffer_free(memBlock1);
    buffer_free(memBlock2);

    buffer_free(memBlock3);



    char  * memBlock6 = buffer_malloc(9999);
//buffer_pool_destory(); //todo  double delete  todo

    free_buffer_pool_print();
    used_buffer_pool_print();

    printf("########111111#############\n");

    buffer_free(memBlock6);



    free_buffer_pool_print();
    used_buffer_pool_print();
    buffer_runtime_print();


#if 0
    char  * memBlock1 = buffer_malloc(6);
    char  * memBlock2 = buffer_malloc(172);
    char  * memBlock3 = buffer_malloc(186);
    char  * memBlock4 = buffer_malloc(1300);
    char  * memBlock5 = buffer_malloc(2100);
    memset(memBlock5, 0,2100);
    printf("########111111#############\n");
    free_buffer_pool_print();
    used_buffer_pool_print();
    printf("########111111#############\n");

#if 1
    buffer_free(memBlock1+get_memblock_head_len());
    buffer_free(memBlock2+get_memblock_head_len());
    buffer_free(memBlock3+get_memblock_head_len());
    buffer_free(memBlock4+get_memblock_head_len());
    buffer_free(memBlock5+get_memblock_head_len());
#endif
    printf("########111111#############\n");

    free_buffer_pool_print();

    used_buffer_pool_print();
    printf("#########11111############\n");

    ///char  * memBlock6 = buffer_malloc(9999);
    //memset(memBlock6, 0,9999);
    //buffer_free(memBlock6);

    ///////////////////////

    printf("#######333333##############\n");
    //buffer_pool_destory();

    //free_buffer_pool_print();

    //used_buffer_pool_print();

    printf("#########3333############\n");
    buffer_runtime_print();
    // buffer_pool_destory();
    //free_buffer_pool_print();
    //used_buffer_pool_print();

#endif

#if 0
    struct rb_root mytree = RB_ROOT;


    printf("press any key to start init and malloc memory pool\n");
    getchar();
    /*
    看代码已经很好的能反映出init如何使用，8字节的申请100个 16字节的，申请500个 .........
    */
    unsigned int a[10] = { 8 ,16 ,32 ,64 ,128 , 256 , 512 , 1024 ,2028 , 4096 };
    unsigned int b[10] = { 100 ,500 ,1000 ,1500 ,2000 , 3000 , 4000 , 5000 ,4000 , 2000 };
    buffer_pool_init(10, a, b);
    int i = 455;
    memory_pool_node * node = NULL;

    memory_pool_node * node1 = buffer_malloc(6);
    memory_pool_node * node2 = buffer_malloc(172);
    memory_pool_node * node3 = buffer_malloc(186);
    memory_pool_node * node4 = buffer_malloc(1300);
    memory_pool_node * node5 = buffer_malloc(2100);
    buffer_malloc(40);
    buffer_malloc(60);
    buffer_malloc(80);
    buffer_malloc(100);
    buffer_malloc(120);
    buffer_malloc(130);
    buffer_malloc(150);
    buffer_malloc(180);
    buffer_malloc(700);
    buffer_malloc(900);
    buffer_runtime_print();
    printf("press any key to free memory pool\n");
    getchar();
    buffer_free(node5);
    buffer_free(node4);
    buffer_free(node3);
    buffer_runtime_print();
    buffer_free(node2);
    buffer_free(node1);
    buffer_runtime_print();
    printf("press any key to destory memory pool\n");
    getchar();
    buffer_pool_destory();
    printf("press any key to quit\n");
    getchar();
#endif
    return 0;
}
