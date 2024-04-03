#include "os.h"

extern void schedule(void);

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

#define MAX_TIMER 10
#define MAX_LEVEL 5 // 定义跳表的最大层数

// 定义跳表节点结构体
typedef struct TimerNode {
    struct timer timer; // 定时器结构体
    struct TimerNode *forward[MAX_LEVEL]; // 指向不同层下一个节点的指针数组
} TimerNode;

// 定义跳表结构体
typedef struct {
    TimerNode head[MAX_LEVEL]; // 头节点数组，每层一个头节点
    int level; // 跳表当前的层数
} TimerSkiplist;
// 定义全局的跳表定时器
TimerSkiplist timer_skiplist;

// 简单的rand函数实现，每次调用返回递增的数值
static uint32_t simple_rand_seed = 0;
int simple_rand() {
    simple_rand_seed = simple_rand_seed * 1103515245 + 12345;
    return (uint32_t)(simple_rand_seed / 65536) % 32768;
}

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

// 初始化跳表定时器
void timer_skiplist_init() {
    for (int i = 0; i < MAX_LEVEL; i++) {
        // 初始化每层的头节点
        timer_skiplist.head[i].timer.func = NULL;
        for (int j = 0; j < MAX_LEVEL; j++) {
            timer_skiplist.head[i].forward[j] = NULL;
        }
    }
    timer_skiplist.level = 0; // 初始化跳表层数为0
	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);
}

// 创建一个新的定时器节点
TimerNode *timer_skiplist_create_node(struct timer *timer_data) {
    static TimerNode timer_nodes[MAX_TIMER]; // 静态数组存放定时器节点
    static int used_nodes = 0; // 已使用的节点数

    if (used_nodes >= MAX_TIMER) {
        return NULL; // 如果所有节点都已使用，返回NULL
    }

    TimerNode *new_node = &timer_nodes[used_nodes++]; // 获取一个未使用的节点
    new_node->timer = *timer_data; // 复制定时器数据
    for (int i = 0; i < MAX_LEVEL; i++) {
        new_node->forward[i] = NULL; // 初始化指针数组
    }
    return new_node;
}

// 生成一个随机层数
static inline int timer_skiplist_random_level() {
    int lv = 1;
    // 随机生成层数，概率为1/2
    while ((simple_rand() % 2) && lv < MAX_LEVEL) {
        lv++;
    }
    return lv;
}

// 向跳表中添加一个定时器
struct timer *timer_skiplist_add(void (*handler)(void *arg), void *arg, uint32_t timeout) {
    if (NULL == handler || 0 == timeout) {
        return NULL; // 参数检查
    }

    struct timer new_timer = {handler, arg, _tick + timeout}; // 创建定时器结构体
    TimerNode *new_node = timer_skiplist_create_node(&new_timer); // 创建新节点
    if (!new_node) {
        return NULL; // 如果节点创建失败，返回NULL
    }

    int level = timer_skiplist_random_level(); // 随机生成新节点的层数
    if (level > timer_skiplist.level) {
        // 如果新节点层数大于当前跳表层数，更新跳表层数
        timer_skiplist.level = level;
    }

    TimerNode *update[MAX_LEVEL]; // 用于记录新节点插入位置的前一个节点
    TimerNode *current = &timer_skiplist.head[level - 1]; // 从最高层的头节点开始
    for (int i = level - 1; i >= 0; i--) {
        // 从最高层开始，找到第 i 层小于且最接近 timeout 的元素
        while (current->forward[i] && current->forward[i]->timer.timeout_tick < new_node->timer.timeout_tick) {
            current = current->forward[i];
        }
        update[i] = current; // 记录位置
    }

    for (int i = 0; i < level; i++) {
        // 更新每一层的指针，将新节点插入进去
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    return &new_node->timer; // 返回新创建的定时器
}

// 从跳表中删除一个定时器
void timer_skiplist_delete(struct timer *timer) {
    TimerNode *update[MAX_LEVEL]; // 用于记录待删除节点的前一个节点
    TimerNode *current = &timer_skiplist.head[timer_skiplist.level - 1]; // 从最高层的头节点开始
    for (int i = timer_skiplist.level - 1; i >= 0; i--) {
        // 从最高层开始，找到第 i 层小于且最接近 timer 的元素
        while (current->forward[i] && &current->forward[i]->timer != timer) {
            current = current->forward[i];
        }
        update[i] = current; // 记录位置
    }

    current = current->forward[0]; // 移动到下一层
    if (current && &current->timer == timer) {
        // 如果找到了定时器，更新每一层的指针，跳过待删除的节点
        for (int i = 0; i < timer_skiplist.level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }
    }
}

// 检查并执行定时器
static inline void timer_skiplist_check() {
    TimerNode *current = &timer_skiplist.head[0]; // 从最低层的头节点开始
    while (current->forward[0] && _tick >= current->forward[0]->timer.timeout_tick) {
        // 如果当前tick大于等于定时器的timeout_tick，则执行定时器函数
        TimerNode *node_to_delete = current->forward[0];
        node_to_delete->timer.func(node_to_delete->timer.arg);

        // 删除定时器节点
        timer_skiplist_delete(&node_to_delete->timer);
        current = &timer_skiplist.head[0]; // 重新从头节点开始检查
    }
}

// 定时器中断处理函数
void timer_skiplist_handler() {
    _tick++; // 增加tick计数
    printf("tick: %d\n", _tick);

    timer_skiplist_check(); // 检查并执行定时器

    timer_load(TIMER_INTERVAL); // 重新加载定时器间隔

    schedule(); // 调度其他任务
}