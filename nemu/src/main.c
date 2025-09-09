void init_monitor(int, char *[]);
//对 初始化监视器的函数 的声明 定义位于 monitor.c
void reg_test();
void restart();
void ui_mainloop();

int main(int argc, char *argv[]) {

	/* Initialize the monitor. */
	init_monitor(argc, argv);
	//调用初始化监视器的函数 

	/* Test the implementation of the `CPU_state' structure. */
	reg_test();
	//此函数用于测试模拟的 CPU 寄存器的功能是否正确

	/* Initialize the virtual computer system. */
	restart();
	//调用重启函数，初始化虚拟计算机系统
	
	/* Receive commands from user. */
	ui_mainloop();

	return 0;
}
