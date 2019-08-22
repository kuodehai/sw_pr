#include "Common/Interfaces/memory_pool.h"
#include "Common/Interfaces/rbtree.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define LOG_RUN_DEBUG(format,...)  do{\
        printf("DEBUG: [%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)

#pragma pack(push, 1)  //后面可改为1, 2, 4, 8

typedef int Type;
typedef  struct my_node {
    struct rb_node	rb_node;
    Type key;	//memoryblockSize
    void* memoryblock;
} memory_pool_node_t,*pMemory_pool_node_t;

typedef struct {
    struct rb_root free_tree ;//空闲节点头指针
    struct rb_root used_tree ;//占用节点头指针
    unsigned int block_len[MAX_COLUME_NUM];
    unsigned int block_total_count[MAX_COLUME_NUM];
    unsigned int block_used_count[MAX_COLUME_NUM];
} memory_pool_t,pMemory_pool_t;

#pragma pack(pop)

static memory_pool_t  g_memory_pool ;
const   int  MEMBLOCK_HEAD_LEN  =   sizeof(struct rb_node) + sizeof(Type);
int  get_memblock_head_len()
{
    return MEMBLOCK_HEAD_LEN;
}

#if 1

#define MAXSIZE 100


//     定义链表插入函数
void InsertList(PNode List,void*  my_node)
{
    PNode PTail = NULL;
    PNode P = List;    //	 定义节点p指向头节点
    while (P != NULL)
    {
        PTail =P;
        P = P->Next;
    }
    PNode Tmp = (PNode)malloc(sizeof(Node));    //    分配一个临时节点用来存储要插入的数据
    if (Tmp == NULL)
    {
        LOG_RUN_DEBUG("内存分配失败！");
        exit(-1);
    }
    //	  插入节点
    Tmp->my_node = my_node;	 //    把数据赋值给节点数据域
    PTail->Next = Tmp;	//	  末尾节点指针指向下一个新节点
    Tmp->Next = NULL; 	   //	 新节点指针指向为空
    PTail = Tmp;	  //	将新节点复制给末尾节点
    LOG_RUN_DEBUG("");
}



//From small to large order search   非递归
struct my_node * InorderSearchMemblockSize(struct rb_node* t, Type key)
{
    struct rb_node* Seqstack[MAXSIZE];
    int top = -1;
    struct rb_node* p;

    if(t != NULL)
    {
        p = t;
        while(top > -1 || p != NULL)
        {
            while(p != NULL)                        // while循环将根结点的最左结点全部压栈
            {
                top ++;
                Seqstack[top] = p;
                p = p->rb_left;
            }
            if(top > -1)                            // 当结点p没有最左结点时出栈
            {
                p = Seqstack[top];
                struct my_node *mynode = container_of(p, struct my_node, rb_node);
                //printf("key=%d ", mynode->key);             // 访问结点p
                if(key <=  mynode->key)
                {
                    return mynode;
                }
                top --;
                p = p->rb_right;                      // 转向处理右孩子结点
            }
        }
    }
    return NULL;
}


//先序遍历非递归
struct my_node *   PreorderSearchMemblock(struct rb_node* t, void* memoryblock)
{
    struct rb_node* Seqstack[MAXSIZE];
    int top = -1;
    struct rb_node* p;
    /////////////////


    LOG_RUN_DEBUG("rb_node=%d	   Type=%d	memory_pool_node_t=%d",sizeof( struct rb_node) ,sizeof(Type),sizeof(memory_pool_node_t));

    ////////////////
    if(t != NULL)
    {
        p = t;
        while(top > -1 || p != NULL)
        {
            while(p != NULL)						 // while循环将根结点的最左结点全部压栈
            {
                top ++;
                Seqstack[top] = p;
                p = p->rb_left;
            }
            if(top > -1)							 // 当结点p没有最左结点时出栈
            {
                p = Seqstack[top];
                struct my_node *mynode = container_of(p, struct my_node, rb_node);
                //printf("key=%d ", mynode->key);			   // 访问结点p
                LOG_RUN_DEBUG("memoryblock=%p	   mynode->memoryblock=%p	memblock_head_len=%d",memoryblock, mynode->memoryblock,sizeof(struct my_node));
                if( (memoryblock + 48 ) ==	mynode->memoryblock)
                {
                    return mynode;
                }
                top --;
                p = p->rb_right;					   // 转向处理右孩子结点
            }
        }
    }
    return NULL;

}
#endif


void  postOrderRecursionDestory(struct rb_root *root, struct rb_node *rbnode)
{
    if (rbnode==NULL)
        return ;
    if (rbnode->rb_left != NULL)
        return postOrderRecursionDestory(root,rbnode->rb_left);
    if (rbnode->rb_right != NULL)
        return postOrderRecursionDestory(root,rbnode->rb_right);

    struct my_node *mynode = container_of(rbnode, struct my_node, rb_node);   // 访问结点rbnode
    if (mynode==NULL)
        return ;
    if (mynode->memoryblock)
    {
        free(mynode->memoryblock);
        mynode->memoryblock = NULL;
    }
    rb_erase(&mynode->rb_node, root);
    free(mynode);
    mynode = NULL;
}


//Insert the mynode into the red black tree.Insert success, return 0;Fail to return -1.
int insertMemBlockNode(struct rb_root *root, struct my_node *mynode)
{
//LOG_RUN_DEBUG("");
    struct rb_node **tmp = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*tmp)
    {
        struct my_node *my = container_of(*tmp, struct my_node, rb_node);

        parent = *tmp;
        if (mynode->key <= my->key)
            tmp = &((*tmp)->rb_left);
        else if (mynode->key > my->key)
            tmp = &((*tmp)->rb_right);
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&mynode->rb_node, parent, tmp);
    rb_insert_color(&mynode->rb_node, root);

    return 0;
}


struct my_node * getMemBlockSizeNode(struct rb_root *root, Type key)
{
    struct my_node *mynode  = NULL;
    if(root == NULL)
    {
        return NULL;
    }
    if ((mynode = InorderSearchMemblockSize(root->rb_node, key)) == NULL)
    {
        printf("free tree not  exist node  memblokSize >= requst memblokSize:%d , malloc new node  memblokSize:%d\n",key,key);
        if ((mynode = malloc(sizeof(struct my_node))) == NULL)
            return NULL;
        mynode->key = key;

        if (( mynode->memoryblock =  (unsigned char *)malloc(mynode->key) ) == NULL)
            return NULL;
        memset(mynode->memoryblock, 0, mynode->key);

        return  mynode;
    }
    printf("free tree  exist node  mynode->key:%d, requst memblokSize:%d\n",mynode->key,key);
    rb_erase(&mynode->rb_node, root);
    //free(mynode);
    return mynode;
}


static void print_rbtree(struct rb_node *tree, Type key, int direction)
{
    if (tree != NULL)
    {
        if (direction == 0)    // tree鏄牴鑺傜偣
        {
            printf("%2d(B) is root\n", key);
        }
        else                // tree鏄垎鏀妭鐐?
        {
            printf("%2d(%s) is %2d's %6s child\n", key, rb_is_black(tree) ? "B" : "R", key, direction == 1 ? "right" : "left");
        }

        if (tree->rb_left)
        {
            print_rbtree(tree->rb_left, rb_entry(tree->rb_left, struct my_node, rb_node)->key, -1);
        }
        if (tree->rb_right)
        {
            print_rbtree(tree->rb_right, rb_entry(tree->rb_right, struct my_node, rb_node)->key, 1);
        }
    }
}


void my_print(struct rb_root *root)
{
    if (root != NULL && root->rb_node != NULL)
        print_rbtree(root->rb_node, rb_entry(root->rb_node, struct my_node, rb_node)->key, 0);
}

int buffer_pool_init(unsigned int colume_no, unsigned int block_len[], unsigned int block_count[])
{
    struct my_node *mynode = NULL;
    for (int i = 0; i < colume_no; i++)
    {
        if( colume_no <= MAX_COLUME_NUM )
        {
            g_memory_pool.block_len[i] = block_len[i];
            g_memory_pool.block_total_count[i] = block_count[i];
            g_memory_pool.block_used_count[i] = 0 ;
        }
        else
        {
            printf("mem_colume_num:%d <  init colume:%d ,please modify MAX_COLUME_NUM !!!!",MAX_COLUME_NUM,colume_no);
        }
        for (int j = 0; j < block_count[i]; j++)
        {
            if ((mynode = malloc(sizeof(struct my_node))) == NULL)
                return -1;
            mynode->key = block_len[i];

            if (( mynode->memoryblock =  (unsigned char *)malloc(mynode->key) ) == NULL)
                return -1;
            memset(mynode->memoryblock, 0, mynode->key);

            insertMemBlockNode(&g_memory_pool.free_tree,mynode); //free_tree
            //my_print(&g_memory_pool.free_tree);
            //printf("\n");
        }

    }
    printf("g_memory_pool.free_tree\n");
    my_print(&g_memory_pool.free_tree);
    printf("\n");
    return 0;


}

void * buffer_malloc(unsigned int memblock_size)
{
    struct my_node *mynode = NULL;
    PNode  PTail ;
    PNode  List = (PNode)malloc(sizeof(Node));    //    创建分配一个头节点内存空间
    if (List == NULL)    //    判断是否分配成功
    {
        printf("空间分配失败 \n");
        exit(-1);
    }
    List->Next = NULL;

    int remain_memblock_size = memblock_size;
    for (int i = MAX_COLUME_NUM-1; i >=0  && remain_memblock_size >  0; i--)
    {
        int block_len = g_memory_pool.block_len[i];
        int  memblock_count = remain_memblock_size / block_len ;
        remain_memblock_size = remain_memblock_size % block_len;
        if(remain_memblock_size > (block_len/2))
        {
            memblock_count++; //need mem
            remain_memblock_size = 0 ;
        }
        if(memblock_count)
        {
            int  free_i_memblock_len_block_count = g_memory_pool.block_total_count[i] - g_memory_pool.block_used_count[i] ;
            if(memblock_count <=  free_i_memblock_len_block_count)
            {
                for( int  j= 0;  j< memblock_count; j++)
                {

                    ///////////////////////////////////////////////////////////
                    mynode = getMemBlockSizeNode(&g_memory_pool.free_tree, block_len) ;  //    //malloc
                    if(mynode)
                    {
                        LOG_RUN_DEBUG("");
                        InsertList(List,mynode);
                        LOG_RUN_DEBUG("");
                        insertMemBlockNode(&g_memory_pool.used_tree,mynode); //used_tree
                        g_memory_pool.block_used_count[i]++;
                    }
                    ///////////////////////////////////////////////////////////
                }
            }
            else
            {
                for( int j= 0; j< free_i_memblock_len_block_count; j++)
                {
                    ///////////////////////////////////////////////////////////
                    mynode = getMemBlockSizeNode(&g_memory_pool.free_tree, block_len);   //    //malloc
                    if(mynode)
                    {
                        InsertList(List,mynode);
                        insertMemBlockNode(&g_memory_pool.used_tree,mynode); //used_tree
                        g_memory_pool.block_used_count[i]++;
                    }
                    ///////////////////////////////////////////////////////////

                }

                int  new_memblock_count = memblock_count -  free_i_memblock_len_block_count;

                for(int j= 0; j<new_memblock_count; j++)
                {

                    printf("free tree not  exist node  memblokSize:%d , malloc new node  memblokSize:%d\n",block_len,block_len);

                    if ((mynode = malloc(sizeof(struct my_node))) == NULL)
                        return NULL;
                    mynode->key = block_len;

                    if (( mynode->memoryblock =  (unsigned char *)malloc(block_len) ) == NULL)
                        return NULL;
                    memset(mynode->memoryblock, 0,block_len);
                    ///////////////////////////////////////////////////////////
                    InsertList(List,mynode);
                    insertMemBlockNode(&g_memory_pool.used_tree,mynode); //used_tree
                    g_memory_pool.block_used_count[i]++;
                    ///////////////////////////////////////////////////////////
                    g_memory_pool.block_total_count[i]++;

                }
            }

        }
    }
    printf("List=%p\n",    List);
    //insertMemBlockNode(&g_memory_pool.used_tree,init_mynode); //used_tree
    return  List ;

}

#if  1
int     buffer_free(void * List_)
{
    if(List_== NULL)
    {
        LOG_RUN_DEBUG("");
        return 1;
    }
    PNode P, Tmp;
    PNode List = (PNode)List_;
    ///////////////////////定义删除整个链表函数////////////////////////
    P = List->Next;    //定义指针P指向链表要删除的链表List的第一个点节点
    List->Next = NULL;
    while (P != NULL) {
        Tmp = P->Next;        //临时Tmp指向要删除的节点的下个节点
        //buffer_free(P->my_node);
        if(P->my_node)
        {

            LOG_RUN_DEBUG("");
            struct my_node *mynode =  (struct my_node *)P->my_node;
            /////////////test//////////////////////
            for (int i = 0; i < MAX_COLUME_NUM; i++)
            {

                if(g_memory_pool.block_len[i] == mynode->key)
                    g_memory_pool.block_used_count[i]--;
            }
            // LOG_RUN_DEBUG("");
            ///////////////////////
            rb_erase(&mynode->rb_node, &g_memory_pool.used_tree);
            insertMemBlockNode(&g_memory_pool.free_tree,mynode); //free_tree
        }

        free(P);    //释放指针P指向的节点
        P = Tmp;    //重新赋值
    }
    /////////////////////////////////////////////////////////////
    printf("删除链表成功！\n");

    return 0;
}

#else


#endif

void destoryTree(struct rb_root *root) //todo
{
    struct rb_node* p = NULL;
    struct rb_node* t = NULL;
    if(root == NULL)
    {
        return;
    }
    struct rb_node* Seqstack[MAXSIZE];
    int top = -1;
    while((t =  root->rb_node) != NULL)
    {
        p = t;

        while(top > -1 || p != NULL)
        {
            while(p != NULL)                        // while循环将根结点的最左结点全部压栈
            {
                top ++;
                Seqstack[top] = p;
                p = p->rb_left;
            }
            if(top > -1)                            // 当结点p没有最左结点时出栈
            {
                p = Seqstack[top];

                struct my_node *mynode = container_of(p, struct my_node, rb_node);
                if(mynode == NULL)
                {
                    break;
                }
                //printf("key=%d ", mynode->key);             // 访问结点p

                rb_erase(&mynode->rb_node, root);
                if(mynode->memoryblock)
                {
                    free(mynode->memoryblock);
                    mynode->memoryblock = NULL;
                    /////////////test//////////////////////
                    for (int i = 0; i < MAX_COLUME_NUM; i++)
                    {
                        if(g_memory_pool.block_len[i] == mynode->key && g_memory_pool.block_used_count[i] > 0)
                            g_memory_pool.block_used_count[i]--;
                    }
                    ///////////////////////
                }
                free(mynode);
                mynode = NULL;

                top --;
                p = p->rb_right;                      // 转向处理右孩子结点
            }
        }
    }

}


int     buffer_pool_destory(void)
{
    destoryTree(&g_memory_pool.free_tree);
    destoryTree(&g_memory_pool.used_tree);
    return 0;
}

int  free_buffer_pool_print(void)
{
    printf("memory_pool.free_tree\n");
    my_print(&g_memory_pool.free_tree);
    printf("\n");
    return 0;
}
int  used_buffer_pool_print(void)
{
    printf("memory_pool.used_tree\n");
    my_print(&g_memory_pool.used_tree);
    printf("\n");
    return 0;
}

int   buffer_runtime_print(void)
{

    printf("\n*********************** memory pool runtime report start************************\n");
    for (int i = 0; i < MAX_COLUME_NUM; i++)
    {
        printf("pool no[%d] blocksize[%4d] blockTotalCount[%4d] usedBlock[%4d] used percentage[%4d%%]\n" \
               , i , g_memory_pool.block_len[i] , g_memory_pool.block_total_count[i] ,g_memory_pool.block_used_count[i] , g_memory_pool.block_used_count[i]*100/ g_memory_pool.block_total_count[i]);
    }
    printf("*********************** memory pool runtime report end**************************\n");
    return 0;

}


