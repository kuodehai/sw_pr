/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

/*
各种池---内存池的高效实现（C语言）https://blog.csdn.net/forever5312/article/details/78029672
Linux使用内存映射文件做内存池  https://blog.csdn.net/cnsword/article/details/7327676
二叉树遍历非递归算法——先序遍历 https://www.cnblogs.com/greedyco/p/7182555.html
 二叉树遍历非递归算法——中序遍历  https://www.cnblogs.com/greedyco/p/7182768.html
 二叉树遍历非递归算法——后序遍历https://www.cnblogs.com/greedyco/p/7187408.html
 C语言描述链表的实现及操作 https://www.cnblogs.com/maluning/p/7966875.html
 使用内存文件映射实现共享内存 https://blog.csdn.net/nb_vol_1/article/details/51916563

 mmap是linux内存映射文件，是将文件映射成为内存地址空间的一种方式，其实，方法很简单。
memfd = open(MEMFILE, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
memd = mmap(NULL, (sizeof(Type)) * size, PROT_WRITE | PROT_READ,
                                    MAP_SHARED, memfd, 0);
                                    //大家完全可以只在映射文件中保存数据，通过不同进程中相同的内存结构来访问。



各种池---内存池的高效实现（C语言?遗留问题

1.init生效后，内存池结构固话，无法动态扩展，这部分后续可以添加，尤其是带有自动学习功能，需要的内存大小多生成，不需要的自动减少。

2.从代码上看，used链表其实没什么卵用，当时设计的主要目的是管理所有内存节点，所有内存即使用户不归还，也可以做一个自动回收机制，回收内存。但是这部分没做。既然没做，所以used目前是可以去除的，意思就是用户申请内存，把这个节点直接给用户，内存池不再管理，如果用户不归还，那就是内存泄露了。。。。这样做有好处，高效。

3.内存使用率打印函数可以增加一些统计功能，比如 用户申请内存不是在池中分配的，最好也记录下来，这样为修改init参数提供依据。

一般直接使用已有固定内存池


todo: 红黑树内存索引，使用内存池来存放两类数据，一类是存放红黑树节点的内存池，一类是存放键值节点的内存池。
使用红黑树维护内存块：一个已经使用内存块红黑树，另一个空闲内存块红黑树。

初始时申请一定大小的内容，赋给自定义节点插入空闲内存块红黑树
自定义节点
struct my_node {
	struct rb_node rb_node;    // 红黑树节点
	Type key;                // 键值  内存块大小
	//void* memoryblock;;						 // ... 用户自定义的数据  指向对应内存块地址首地址(动态获取，如果空闲内存块已使用完，
									  申请新的内存块放入空闲内存块红黑树，申请的总内存要有限制，使用的内存块放入已经使用内存块红黑树）
};
*/

typedef struct ListNode {
    void*  my_node  ;        //    数据域，存放数据
    struct ListNode *  Next;        //    指向下一个链表节点
} Node, *PNode;

//     定义链表插入函数
void InsertList(PNode List,void*  my_node);
//定义删除整个链表函数
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
