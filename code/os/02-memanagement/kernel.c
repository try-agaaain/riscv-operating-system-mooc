#include "os.h"

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
extern void uart_init(void);
extern void page_init(void);
extern void malloc_init();
extern void bmalloc_test();
void start_kernel(void)
{
	uart_init();
	uart_puts("Hello, RVOS!\n");

	// bmalloc_test();
	page_init();
	page_test();
	while (1) {}; // stop here!
}

