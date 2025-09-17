#include "monitor/monitor.h"
#include "cpu/helper.h"
#include "monitor/watchpoint.h"
#include <setjmp.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
//翻译：当执行的指令数少于此值时，指令的汇编代码仅输出到屏幕上，这在使用 `si` 命令时很有用，你可以根据需要修改此值。
#define MAX_INSTR_TO_PRINT 10

int nemu_state = STOP;
//初始的 nemu_state 状态为 STOP，表示模拟器当前处于停止状态。

int exec(swaddr_t);
//声明函数 exec，参数为 swaddr_t 类型，返回值为 int 类型
//返回值是变化的，表示执行的指令长度。

char assembly[80];
char asm_buf[128];

/* Used with exception handling. */
jmp_buf jbuf;

void print_bin_instr(swaddr_t eip, int len) {
	int i;
	int l = sprintf(asm_buf, "%8x:   ", eip);
	for(i = 0; i < len; i ++) {
		l += sprintf(asm_buf + l, "%02x ", instr_fetch(eip + i, 1));
	}
	sprintf(asm_buf + l, "%*.s", 50 - (12 + 3 * len), "");
}

/* This function will be called when an `int3' instruction is being executed. */
void do_int3() {
	printf("\nHit breakpoint at eip = 0x%08x\n", cpu.eip);
	nemu_state = STOP;
}

/* Simulate how the CPU works. */
void cpu_exec(volatile uint32_t n) {
	//定义函数 cpu_exec，参数为一个 易变的 32位无符号整数 n。
	//易变：表示该变量可能在程序的其他部分被修改。作用是告诉编译器不要对该变量进行优化，以确保每次访问时都从内存中读取最新的值
	if(nemu_state == END) {
		printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
		return;
	}
	//如果 nemu_state 的值为 END，则表示程序已经结束执行，输出提示信息并返回
	nemu_state = RUNNING;
	//否则将 nemu_state 的值设置为 RUNNING，表示程序正在运行
#ifdef DEBUG
	volatile uint32_t n_temp = n;
#endif
	//ifdef和endif之间的代码仅在 宏名 DEBUG 被定义时编译
	//若DEBUG被定义，则定义一个易变的32位无符号整数 n_temp，并将 n 的值赋给 n_temp
	setjmp(jbuf);

	for(; n > 0; n --) {
		//函数执行次数为n。
#ifdef DEBUG
		swaddr_t eip_temp = cpu.eip; //swaddr_t 在 common.h 中被定义为 uint32_t 类型
		//定义一个 swaddr_t 类型（uint32_t类型）的变量 eip_temp，并将 CPU 的指令指针赋值给它
		if((n & 0xffff) == 0) {
			//0xffff转换为二进制为16个1，即1111111111111111，若他与n按位与的结果为0，则表示n是65536的倍数。
			//也就是说，if的条件每65536次n变化一次时成立
			//若n为-1，转化为二进制是一个极大数，会使条件成立最多次。
			/* Output some dots while executing the program. */
			//翻译：在执行程序时输出一些点
			fputc('.', stderr);
			//fputc的定义：int fputc(int char, FILE *stream)，也就是将字符char写入到流stream中
		}//当n是65536的倍数时，向流 stderr 写入一个点。
#endif

		/* Execute one instruction, including instruction fetch,
		 * instruction decode, and the actual execution. */
		//翻译：执行一条指令，包括指令获取、指令解码和实际执行
		int instr_len = exec(cpu.eip);
		//定义int类型的变量 instr_len，并将 exec 函数的返回值赋给它。
		cpu.eip += instr_len;
		//将 CPU 的指令指针寄存器 eip 增加 instr_len，指向下一条指令的地址
		//这实际上是模拟了 CPU 执行指令后的行为，即更新指令指针以指向下一条指令

#ifdef DEBUG
		print_bin_instr(eip_temp, instr_len);
		strcat(asm_buf, assembly);
		Log_write("%s\n", asm_buf);
		if(n_temp < MAX_INSTR_TO_PRINT) {
			printf("%s\n", asm_buf);
		}
#endif

		/* TODO: check watchpoints here. */
#ifdef DEBUG
// 在调试模式下检查监视点
WP* current_wp = get_head_wp();
while (current_wp != NULL) {
    bool success;
    uint32_t new_value = expr(current_wp->expr, &success);
    if (success && new_value != current_wp->value) {
        // 监视点值发生变化，触发监视点
        printf("\nHint watchpoint %d at address 0x%08x\n", current_wp->NO, eip_temp);
        printf("  %s\n", current_wp->expr);
        printf("  Old value = %u\n  New value = %u\n", current_wp->value, new_value);
        current_wp->value = new_value; // 更新值
        nemu_state = STOP;
        return;
    }
    current_wp = current_wp->next;
}
#endif

#ifdef HAS_DEVICE
		extern void device_update();
		device_update();
#endif

		if(nemu_state != RUNNING) { return; }
	}

	if(nemu_state == RUNNING) { nemu_state = STOP; }
}
