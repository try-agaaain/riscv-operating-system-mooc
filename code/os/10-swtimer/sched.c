#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];

/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

// os.h 文件中添加
#define MAX_PRIORITY_LEVELS 3
#define TIME_QUANTUM {2, 4, 8} // 时间片配额，每个优先级的时间片数量

struct task_struct {
    struct context ctx; // 任务的上下文信息
    int priority;       // 任务的当前优先级
    int time_slice;     // 任务的剩余时间片
    int valid;          // 任务是否有效
};
struct task_struct ctx_tasks[MAX_TASKS];


struct priority_queue {
    struct task_struct *tasks[MAX_TASKS]; // 优先级队列中的任务数组
    int task_count;                       // 队列中的任务数量
};

// 优先级队列数组，每个元素代表一个优先级队列
struct priority_queue priority_queues[MAX_PRIORITY_LEVELS];

// 当前运行的任务
struct task_struct *current_task = NULL;

// 时间片配额数组
const int time_quantum[MAX_PRIORITY_LEVELS] = TIME_QUANTUM;

void sched_init()
{
	w_mscratch(0);

	/* enable machine-mode software interrupts. */
	w_mie(r_mie() | MIE_MSIE);
}

struct task_struct *dequeue_task(struct priority_queue *queue) {
    if (queue->task_count == 0) {
        return NULL;
    }

    struct task_struct *task = queue->tasks[0];
    // 将队列中的其他任务向前移动一位
    for (int i = 1; i < queue->task_count; i++) {
        queue->tasks[i - 1] = queue->tasks[i];
    }
    queue->task_count--;

    return task;
}

void enqueue_task(struct task_struct *task) {
    int priority = task->priority;
    if (priority_queues[priority].task_count < MAX_TASKS) {
        priority_queues[priority].tasks[priority_queues[priority].task_count++] = task;
    } else {
        panic("Priority queue is full!");
    }
}

/*
 * implment a simple cycle FIFO schedular
 */
// os.c 文件中添加或修改
void schedule() {
    if(current_task)
        printf("剩余时间片：%d\n", current_task->time_slice);

    if (current_task && (--(current_task->time_slice) > 0)) {
        // 如果当前任务还有剩余时间片，则继续运行
		// printf("return\n");
        return;
    }

    if (current_task && current_task->time_slice <= 0) {
        // 如果当前任务时间片耗尽，降低其优先级（如果不是最低优先级）
        if (current_task->priority < MAX_PRIORITY_LEVELS - 1) {
            current_task->priority++;
        }
        current_task->time_slice = time_quantum[current_task->priority];
        enqueue_task(current_task); // 将当前任务重新放入队列
    }

    // 选择下一个要运行的任务
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (priority_queues[i].task_count > 0) {
            current_task = dequeue_task(&priority_queues[i]);
            // printf("addr: %x, slice: %d", current_task, current_task->time_slice);
            break;
        }
    }
	struct context *p = &current_task->ctx;
	printf("switch: %x\n", p->pc);
    // 切换到选定的任务
    if (current_task) {
        switch_to(&current_task->ctx);
    } else {
        panic("No more tasks to schedule!");
    }
}

/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 */
int task_create(void (*start_routine)(void)) {
    if (_top < MAX_TASKS) {
        struct task_struct *new_task = &ctx_tasks[_top];
        new_task->ctx.sp = (reg_t) &task_stack[_top][STACK_SIZE];
        new_task->ctx.pc = (reg_t) start_routine;
        new_task->priority = 0; // 新任务开始时具有最高优先级
        new_task->time_slice = time_quantum[0]; // 初始化时间片
        new_task->valid = 1; // 标记为有效任务
        enqueue_task(new_task);
        _top++;
        return 0;
    } else {
        return -1;
    }
}
/*
 * DESCRIPTION
 * 	task_yield()  causes the calling task to relinquish the CPU and a new 
 * 	task gets to run.
 */
void task_yield()
{
	/* trigger a machine-level software interrupt */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

