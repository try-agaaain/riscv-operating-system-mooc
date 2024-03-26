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

void user_task1(char param)
{
	uart_puts("Task 1: Created!\n");
	int count = 0;
	while (1) {
		printf("Task 1: count is %d, param is %c, Running...\n", count++, param);
		task_delay(DELAY);
		task_yield(1);
	}
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	char id = 'j';
	printf("id address: %x, value is %c", &id, id);
	task_create(user_task0, NULL);
	task_create(user_task1, id);
}

