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
struct context ctx_tasks[MAX_TASKS];

/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

static void w_mscratch(reg_t x)
{
	/*
	"csrw mscratch, %0" : : "r" (x) 是内嵌汇编的语法。%0 是一个占位符，它在实际的汇编代码中会被替换为一个操作数。
	这里 %0 会被替换为由变量 x 提供的值。"r" (x) 告诉编译器 x 是一个输入操作数，它应该被放在一个通用寄存器中。
	编译器会选择一个寄存器，将 x 的值放入其中，并在实际的汇编指令中使用这个寄存器来替换 %0。
	*/
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

void sched_init()
{
	w_mscratch(0);
}

/*
 * implment a simple cycle FIFO schedular
 */
void schedule()
{
	if (_top <= 0) {
		panic("Num of task should be greater than zero!");
		return;
	}

	_current = (_current + 1) % _top;
	struct context *next = &(ctx_tasks[_current]);
	switch_to(next);
}

/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 */
int task_create(void (*start_routin)(void* param), void* param)
{
	if (_top < MAX_TASKS) {
		// 将栈的位置初始化为栈顶，栈在使用的过程中栈顶指针下移
		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
		ctx_tasks[_top].ra = (reg_t) start_routin;
		ctx_tasks[_top].a0 = (reg_t) param;
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
void task_yield(int task_id)
{
	printf("%d号任务即将被切换！\n", task_id);
	schedule();
	printf("%d号任务已恢复执行！\n", task_id);
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

