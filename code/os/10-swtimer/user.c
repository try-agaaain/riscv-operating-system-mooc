#include "os.h"

#define DELAY 4000

struct userdata {
	int counter;
	char *str;
};

/* Jack must be global */
struct userdata person0 = {0, "Jack"};
struct userdata person1 = {0, "Mark"};
struct userdata person2 = {0, "Jay"};


void timer_func(void *arg)
{
	if (NULL == arg)
		return;

	struct userdata *param = (struct userdata *)arg;

	param->counter++;
	printf("======> TIMEOUT: %s: %d\n", param->str, param->counter);
}

void user_task0(void)
{
	uart_puts("Task 0: Created!\n");

	struct timer *t1 = timer_create(timer_func, &person0, 3);    // 第3个tick触发
	if (NULL == t1) {
		printf("timer_create() failed!\n");
	}
	struct timer *t2 = timer_create(timer_func, &person1, 1);    // 第1个个tick触发
	if (NULL == t2) {
		printf("timer_create() failed!\n");
	}
	struct timer *t3 = timer_create(timer_func, &person2, 6);    // 第6个tick触发
	if (NULL == t3) {
		printf("timer_create() failed!\n");
	}
	while (1) {
		uart_puts("Task 0: Running... \n");
		task_delay(DELAY);
	}
}

void user_task1(void)
{
	uart_puts("Task 1: Created!\n");
	while (1) {
		uart_puts("Task 1: Running... \n");
		task_delay(DELAY);
	}
}

void user_task2(void)
{
	uart_puts("Task 2: Created!\n");
	while (1) {
		uart_puts("Task 2: Running... \n");
		task_delay(DELAY);
	}
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	printf("task0: %x, task1: %x\n", user_task0, user_task1);
	task_create(user_task0);
	task_create(user_task1);
	task_create(user_task2);
}

