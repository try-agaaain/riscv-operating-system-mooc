#include "os.h"

#define DELAY 1000

void user_task0(void)
{
	int count = 0;
	uart_puts("Task 0: Created!\n");
	while (1) {
		printf("Task 0: count is %d, Running...\n", count++);
		task_delay(DELAY);
		task_yield(0);
	}
}

void user_task1(void)
{
	uart_puts("Task 1: Created!\n");
	int count = 0;
	while (1) {
		printf("Task 1: count is %d, Running...\n", count++);
		task_delay(DELAY);
		task_yield(1);
	}
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	task_create(user_task0);
	task_create(user_task1);
}

