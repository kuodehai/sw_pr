#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
/*
���ֳ�---�ڴ�صĸ�Чʵ�֣�C���ԣ�https://blog.csdn.net/forever5312/article/details/78029672
Linuxʹ���ڴ�ӳ���ļ����ڴ��  https://blog.csdn.net/cnsword/article/details/7327676
�����������ǵݹ��㷨����������� https://www.cnblogs.com/greedyco/p/7182555.html
 �����������ǵݹ��㷨�����������  https://www.cnblogs.com/greedyco/p/7182768.html
 �����������ǵݹ��㷨�����������https://www.cnblogs.com/greedyco/p/7187408.html
 C�������������ʵ�ּ����� https://www.cnblogs.com/maluning/p/7966875.html
 ʹ���ڴ��ļ�ӳ��ʵ�ֹ����ڴ� https://blog.csdn.net/nb_vol_1/article/details/51916563

 mmap��linux�ڴ�ӳ���ļ����ǽ��ļ�ӳ���Ϊ�ڴ��ַ�ռ��һ�ַ�ʽ����ʵ�������ܼ򵥡�
memfd = open(MEMFILE, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
memd = mmap(NULL, (sizeof(Type)) * size, PROT_WRITE | PROT_READ,
                                    MAP_SHARED, memfd, 0);
                                    //�����ȫ����ֻ��ӳ���ļ��б������ݣ�ͨ����ͬ��������ͬ���ڴ�ṹ�����ʡ�



���ֳ�---�ڴ�صĸ�Чʵ�֣�C���ԣ ��������

1.init��Ч���ڴ�ؽṹ�̻����޷���̬��չ���ⲿ�ֺ���������ӣ������Ǵ����Զ�ѧϰ���ܣ���Ҫ���ڴ��С�����ɣ�����Ҫ���Զ����١�

2.�Ӵ����Ͽ���used������ʵûʲô���ã���ʱ��Ƶ���ҪĿ���ǹ��������ڴ�ڵ㣬�����ڴ漴ʹ�û����黹��Ҳ������һ���Զ����ջ��ƣ������ڴ档�����ⲿ��û������Ȼû��������usedĿǰ�ǿ���ȥ���ģ���˼�����û������ڴ棬������ڵ�ֱ�Ӹ��û����ڴ�ز��ٹ�������û����黹���Ǿ����ڴ�й¶�ˡ��������������кô�����Ч��

3.�ڴ�ʹ���ʴ�ӡ������������һЩͳ�ƹ��ܣ����� �û������ڴ治���ڳ��з���ģ����Ҳ��¼����������Ϊ�޸�init�����ṩ���ݡ�

һ��ֱ��ʹ�����й̶��ڴ��


todo: ������ڴ�������ʹ���ڴ��������������ݣ�һ���Ǵ�ź�����ڵ���ڴ�أ�һ���Ǵ�ż�ֵ�ڵ���ڴ�ء�
ʹ�ú����ά���ڴ�飺һ���Ѿ�ʹ���ڴ����������һ�������ڴ��������

��ʼʱ����һ����С�����ݣ������Զ���ڵ��������ڴ������
�Զ���ڵ�
struct my_node {
	struct rb_node rb_node;    // ������ڵ�
	Type key;                // ��ֵ  �ڴ���С
	//void* memoryblock;;						 // ... �û��Զ��������  ָ���Ӧ�ڴ���ַ�׵�ַ(��̬��ȡ����������ڴ����ʹ���꣬
									  �����µ��ڴ���������ڴ����������������ڴ�Ҫ�����ƣ�ʹ�õ��ڴ������Ѿ�ʹ���ڴ��������
};
*/

typedef struct ListNode {
    void*  my_node  ;        //    �����򣬴������
    struct ListNode *  Next;        //    ָ����һ������ڵ�
} Node, *PNode;

//     ����������뺯��
void InsertList(PNode List,void*  my_node);
//����ɾ������������
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
