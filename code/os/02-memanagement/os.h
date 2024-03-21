#ifndef __OS_H__
#define __OS_H__

#include "types.h"
#include "platform.h"

#include <stddef.h>
#include <stdarg.h>

/* uart */
extern int uart_putc(char ch);
extern void uart_puts(char *s);

/* printf */
extern int  printf(const char* s, ...);
extern void panic(char *s);

/* memory management */
extern void *page_alloc(int npages);
void page_free(void *p, uint32_t num_pages);
extern void heap_init();
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void heap_test();

#endif /* __OS_H__ */
