#include "os.h"

#define ALIGNMENT 8 // 假设所有的内存块都需要按照8字节对齐
#define PAGE_SIZE 4096
#define PAGE_ORDER 12

// 内存块的头部，用于记录内存块的大小和分配状态
struct BlockHeader {
    uint32_t size; // 内存块的大小，包括头部
    struct BlockHeader *next; // 指向下一个内存块的指针
    uint8_t is_free; // 这个内存块是否是空闲的
};

// 内存块头部的大小需要按照对齐要求来调整
#define BLOCK_HEADER_SIZE ((sizeof(struct BlockHeader) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// 堆的起始地址
static struct BlockHeader *malloc_start = NULL;

// 初始化堆
void malloc_init() {
    malloc_start = (struct BlockHeader *)page_alloc(1); // 分配一页作为堆的起始
    if (malloc_start) {
        malloc_start->size = PAGE_SIZE - BLOCK_HEADER_SIZE;
        malloc_start->next = NULL;
        malloc_start->is_free = 1;
    }
}

// 分配内存
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // 调整请求的大小以满足对齐要求
    total_size = (size + BLOCK_HEADER_SIZE + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    struct BlockHeader *current = malloc_start;
    struct BlockHeader *pre = malloc_start;
    while (current) {
        // 检查当前块是否足够大，且未被占用
        printf("current->size: %d\n", current->size);
        if (current->is_free && current->size >= total_size) {
            // 如果当前块过大，分割它
            // malloc分配的每个块包含头部和分配空间两部分，所以这里是size + BLOCK_HEADER_SIZE
            if (current->size > size) {
                struct BlockHeader *remain_block = (struct BlockHeader *)((uint8_t *)current + total_size);
                remain_block->size = current->size - total_size - BLOCK_HEADER_SIZE;
                remain_block->next = current->next;
                remain_block->is_free = 1;

                current->size = total_size - BLOCK_HEADER_SIZE;
                current->next = remain_block;
            }

            current->is_free = 0;
            return (void *)((uint8_t *)current + BLOCK_HEADER_SIZE);
        }
        pre = current;
        current = current->next;
    }

    // 没有找到合适的块，需要分配新的页
    int need_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;    // 所需的页数
    struct BlockHeader *new_page = (struct BlockHeader *)page_alloc(need_pages);
    printf("当前页剩余空间不够分配，需申请新的页，新页地址：%x\n", new_page);
    if (!new_page) {
        return NULL; // 分配失败
    }

    // 初始化新分配的页
    new_page->size = PAGE_SIZE * need_pages;
    new_page->next = NULL;
    new_page->is_free = 0;
    pre->next = new_page;


    // 如果新页太大，分割它
    if (new_page->size > size + BLOCK_HEADER_SIZE) {
        struct BlockHeader *remain_block = (struct BlockHeader *)((uint8_t *)new_page + size);
        remain_block->size = new_page->size - size;
        remain_block->next = NULL;
        remain_block->is_free = 1;

        new_page->size = size;
        new_page->next = remain_block;
    }

    return (void *)((uint8_t *)new_page + BLOCK_HEADER_SIZE);
}

void free(void *ptr) {
    if (!ptr) {
        return; // 如果指针为空，不执行任何操作
    }

    // 获取要释放的内存块的头部
    struct BlockHeader *block_to_free = (struct BlockHeader *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    block_to_free->is_free = 1; // 标记为自由

    // 合并前后相邻的自由块
    struct BlockHeader *current = malloc_start;
    while (current) {
        if (current->is_free) {
            struct BlockHeader *next = current->next;
            // 如果下一个块是自由的，并且它们在内存中是连续的，则合并它们
            while (next && next->is_free && (uint8_t *)current + current->size == (uint8_t *)next) {
                current->size += next->size;
                current->next = next->next;
                next = current->next;
            }
        }
        current = current->next;
    }

    // 检查是否有足够大的自由块可以释放页
    current = malloc_start;
    while (current) {
        if (current->is_free && current->size >= PAGE_SIZE) {
            uint32_t num_pages = current->size / PAGE_SIZE;


            
            // 计算当前块的起始地址是否是页对齐的
            uint32_t start_addr = (uint32_t)current;
            uint32_t end_addr = start_addr + current->size + BLOCK_HEADER_SIZE;
            if ((start_addr & (PAGE_SIZE - 1)) == 0) {
                // 当前块的结束地址是页对齐的，则这个节点占用连续且完整的页，直接释放
                if((end_addr & (PAGE_SIZE - 1)) == 0) {
                    // 更新链表
                    if (current == malloc_start) {
                        malloc_start = current->next;
                    } else {
                        struct BlockHeader *prev = malloc_start;
                        while (prev && prev->next != current) {
                            prev = prev->next;
                        }
                        if (prev) {
                            prev->next = current->next;
                        }
                    }
                } else {
                    // 结束地址不是页对齐的，即该节点占用的最后一个页不完整，
                    // 则释放前面的页，在最后一个页的起始位置创建新的节点
                    struct BlockHeader *remain_block = (struct BlockHeader *)((uint8_t *)current + num_pages * PAGE_SIZE);
                    remain_block->size = current->size - num_pages * PAGE_SIZE;
                    remain_block->next = current->next;
                    remain_block->is_free = 1;
                    // 更新链表
                    if (current == malloc_start) {
                        malloc_start = remain_block;
                    } else {
                        struct BlockHeader *prev = malloc_start;
                        while (prev && prev->next != current) {
                            prev = prev->next;
                        }
                        if (prev) {
                            prev->next = remain_block;
                        }
                    }
                }
                // 释放整个页
                page_free((void *)start_addr, num_pages);
            }
        }
        current = current->next;
    }
}

// 测试函数
void malloc_test() {
    void *a = malloc(100);
    printf("Allocated %x\n", a);
    // free(a);

    void *b = malloc(1000);
    printf("Allocated %x\n", b);
    free(b);

    void *c = malloc(5000);
    printf("Allocated %x\n", c);
    free(c);
}