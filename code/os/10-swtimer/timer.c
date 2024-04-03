#include "os.h"

extern void schedule(void);

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

#define MAX_TIMER 100

// 定时器最小堆
static struct timer timer_heap[MAX_TIMER];
static int timer_heap_size = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval) {
    /* each CPU has a separate source of timer interrupts. */
    int id = r_mhartid();
    
    *(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

// 初始化定时器堆
void timer_init() {
    timer_heap_size = 0;
    for (int i = 0; i < MAX_TIMER; i++) {
        timer_heap[i].func = NULL; // 使用.func作为标志，表示项是否被使用
        timer_heap[i].arg = NULL;
    }

    /* On reset, mtime is cleared to zero, but the mtimecmp registers 
     * are not reset. So we have to init the mtimecmp manually.
     */
    timer_load(TIMER_INTERVAL);

    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);
}

// 向下调整堆
void percolate_down(int hole) {
    int child;
    struct timer tmp = timer_heap[hole];

    for (; hole * 2 + 1 < timer_heap_size; hole = child) {
        child = hole * 2 + 1;
        // 选择两个子节点中较小的一个
        if (child != timer_heap_size - 1 && timer_heap[child + 1].timeout_tick < timer_heap[child].timeout_tick) {
            child++;
        }
        // 如果子节点更小，则向下移动子节点
        if (timer_heap[child].timeout_tick < tmp.timeout_tick) {
            timer_heap[hole] = timer_heap[child];
        } else {
            break;
        }
    }
    timer_heap[hole] = tmp;
}

// 向上调整堆
void percolate_up(int hole) {
    struct timer tmp = timer_heap[hole];
    int parent = (hole - 1) / 2;

    while (hole > 0 && timer_heap[parent].timeout_tick > tmp.timeout_tick) {
        timer_heap[hole] = timer_heap[parent];
        hole = parent;
        parent = (hole - 1) / 2;
    }
    timer_heap[hole] = tmp;
}

// 创建定时器
struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout) {
    if (NULL == handler || 0 == timeout) {
        return NULL;
    }

    // 使用锁来保护共享的timer_heap
    spin_lock();

    if (timer_heap_size >= MAX_TIMER) {
        spin_unlock();
        return NULL;
    }

    // 设置定时器属性
    int hole = timer_heap_size++;
    timer_heap[hole].func = handler;
    timer_heap[hole].arg = arg;
    timer_heap[hole].timeout_tick = _tick + timeout;

    // 向上调整堆，以保持最小堆的性质
    percolate_up(hole);

    spin_unlock();

    return &timer_heap[hole];
}

// 删除定时器
void timer_delete(struct timer *timer) {
    spin_lock();

    for (int i = 0; i < timer_heap_size; i++) {
        if (&timer_heap[i] == timer) {
            // 将最后一个元素移动到当前位置
            timer_heap[i] = timer_heap[--timer_heap_size];
            // 向下或向上调整堆
            percolate_down(i);
            percolate_up(i);
            break;
        }
    }

    spin_unlock();
}

// 检查并执行定时器
static inline void timer_check() {
    while (timer_heap_size > 0 && timer_heap[0].timeout_tick <= _tick) {
        struct timer top_timer = timer_heap[0];
        // 删除堆顶元素
        timer_heap[0] = timer_heap[--timer_heap_size];
        percolate_down(0);

        // 执行定时器回调
        if (top_timer.func) {
            top_timer.func(top_timer.arg);
        }
    }
}

// 定时器中断处理函数
void timer_handler() {
    _tick++;
    printf("tick: %d\n", _tick);

    timer_check();

    timer_load(TIMER_INTERVAL);

    schedule();
}