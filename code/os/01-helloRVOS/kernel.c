extern void uart_init(void);
extern void uart_puts(char *s);
extern int uart_getc();
extern char* uart_read_line(char* buffer, int max_length);
extern void echo(char* s);

void start_kernel(void)
{
    uart_init(); // 初始化UART
	uart_puts("Hello, RVOS!\n");

    const int MAX_INPUT_LENGTH = 256;
    char input_buffer[MAX_INPUT_LENGTH];
	while(1){
		uart_puts("Please enter a line: ");
		uart_read_line(input_buffer, MAX_INPUT_LENGTH); // 读取一行文本
		uart_puts("\nYou entered: ");
		echo(input_buffer); // 显示输入的文本
	}
    return 0;
}
