#include "nemu.h"

#define ENTRY_START 0x100000

extern uint8_t entry [];
extern uint32_t entry_len;
extern char *exec_file;

void load_elf_tables(int, char *[]);
void init_regex();
void init_wp_pool();
void init_ddr3();

FILE *log_fp = NULL; //定义日志文件指针 *log_fp 最初值为 NULL；FILE 的意义是文件流结构体，包含了文件操作的各种信息。
                     //所有文件操作相关的函数 都用FILE*类型的指针作为参数

 static void init_log() {
	log_fp = fopen("log.txt", "w");  //fopen函数用于打开一个文件，并返回一个指向该文件的文件指针
	                                 //"w"表示以写入模式打开文件，如果文件不存在，则创建该文件 这是w模式的特点。但是如果出于权限等原因无法创建或打开文件，fopen会返回NULL
									 //这一步过后，log_fp指向了一个可以写入的文件流
	Assert(log_fp, "Can not open 'log.txt'");//assert宏用于检查log_fp是否为NULL，如果为NULL则表示文件打开失败，程序会输出错误信息并终止运行
}


static void welcome() {
	printf("Welcome to NEMU!\nThe executable is %s.\nFor help, type \"help\"\n",
			exec_file);
}

void init_monitor(int argc, char *argv[]) {
	/* Perform some global initialization */
	//翻译：执行一些全局初始化

	/* Open the log file. */
	//翻译：打开日志文件 
	init_log(); //执行初始化日志的函数 此函数定义位于本文件第15行 

	/* Load the string table and symbol table from the ELF file for future use. */
	//翻译：从 ELF 文件加载字符串表和符号表以供将来使用
	//ELF文件是一种可执行文件格式，包含了程序的机器代码、数据段、符号表等信息
	//字符串表存储了程序中使用的各种字符串，符号表则包含了程序中定义的变量和函数的名称及其对应的地址
	//加载这些表格可以帮助调试器在调试过程中更好地理解程序的结构和内容
	load_elf_tables(argc, argv); //执行加载 ELF 表的函数 此函数定义位于 elf.c

	/* Compile the regular expressions. */
	init_regex();

	/* Initialize the watchpoint pool. */
	init_wp_pool();

	/* Display welcome message. */
	welcome();
}

#ifdef USE_RAMDISK
static void init_ramdisk() {
	//定义一个静态函数 init_ramdisk
	int ret;
	const int ramdisk_max_size = 0xa0000;
	//定义一个常量 ramdisk_max_size
	FILE *fp = fopen(exec_file, "rb");
	//定义 FILE 类型的指针 fp，并使用 fopen 函数以二进制、只读模式 ("rb") 打开由 exec_file 指定的文件
	Assert(fp, "Can not open '%s'", exec_file);
	//调用 assert 函数，检查文件指针 fp 是否为 NULL，如果为 NULL 则表示文件打开失败，输出错误信息并终止程序运行
	fseek(fp, 0, SEEK_END);
	//调用 fseek 函数将文件指针移动到文件末尾 参数0表示偏移量为0，SEEK_END表示从文件末尾开始计算偏移量 目的是为了获取文件的总大小
	//fseek函数的作用是设置文件流的当前位置，通常用于随机访问文件 或者说 指定访问文件的某个位置
	//fseek()函数的返回值是一个整数​，用于指示操作是否成功。 如果返回0，表示操作成功；如果返回非零值（通常为-1），表示操作失败
	size_t file_size = ftell(fp);
	//调用 ftell 函数获取当前文件指针的位置（即文件大小），并将其赋值给 file_size 变量
	//ftell函数的作用是返回文件流的当前位置（以字节为单位），通常用于获取文件的大小或当前位置 如果函数调用失败，通常会返回-1L，并设置errno以指示错误类型
	Assert(file_size < ramdisk_max_size, "file size(%zd) too large", file_size);
	//调用 assert 函数，检查 file_size 是否小于 ramdisk_max_size，如果不满足条件则输出错误信息并终止程序运行
	//%zd 是 C 语言中的格式说明符，用于在格式化字符串中表示 size_t 类型的变量

	fseek(fp, 0, SEEK_SET);
	//调用 fseek 函数将文件指针重新移动到文件开头 参数0表示偏移量为0，SEEK_SET表示从文件开头开始计算偏移量
	//这一步是为了准备从文件开头开始读取数据
	ret = fread(hwa_to_va(0), file_size, 1, fp);
	//作用：调用 fread 函数从文件中读取数据，并将其存储到内存中
	assert(ret == 1);
	//调用 assert 函数，检查 ret 是否等于 1，如果不等于 1 则表示读取失败，程序会输出错误信息并终止运行
	fclose(fp);
	//调用 fclose 函数关闭文件指针 fp，释放相关资源
}
#endif

static void load_entry() {
	//定义一个静态函数 load_entry 
	int ret;
	FILE *fp = fopen("entry", "rb");
	//定义 FILE 类型的指针 fp，并使用 fopen 函数以二进制、只读模式 ("rb") 打开名为 "entry" 的文件
	Assert(fp, "Can not open 'entry'");
	//调用 assert 函数，检查文件指针 fp 是否为 NULL，如果为 NULL 则表示文件打开失败，输出错误信息并终止程序运行
	fseek(fp, 0, SEEK_END);
	//调用 fseek 函数将文件指针移动到文件末尾 参数0表示偏移量为0，SEEK_END表示从文件末尾开始计算偏移量 目的是为了获取文件的总大小
	size_t file_size = ftell(fp);
	//调用 ftell 函数获取当前文件指针的位置（即文件大小），并将其赋值给 file_size 变量

	fseek(fp, 0, SEEK_SET);
	//调用 fseek 函数将文件指针重新移动到文件开头 参数0表示偏移量为0，SEEK_SET表示从文件开头开始计算偏移量
	ret = fread(hwa_to_va(ENTRY_START), file_size, 1, fp);
	//调用 fread 函数从文件中读取数据，并将其存储到内存中
	assert(ret == 1);
	//调用 assert 函数，检查 ret 是否等于 1，如果不等于 1 则表示读取失败，程序会输出错误信息并终止运行
	fclose(fp);
}

void restart() { 
	/* Perform some initialization to restart a program */
	//翻译：执行一些初始化以重新启动程序
#ifdef USE_RAMDISK
	/* Read the file with name `argv[1]' into ramdisk. */
	init_ramdisk();
#endif 
	//如果定义了 USE_RAMDISK 宏，则调用 init_ramdisk 函数将指定文件加载到内存中
	/* Read the entry code into memory. */ 
	load_entry();

	/* Set the initial instruction pointer. */
	cpu.eip = ENTRY_START; //设置 CPU 的指令指针寄存器 eip 的初始值为 ENTRY_START（0x100000），也就是内存的起始地址

	/* Initialize DRAM. */
	init_ddr3();
	//调用 init_ddr3 函数初始化 DRAM，根据定义，DRAM 初始化包括将所有行缓冲区的 valid 字段设置为 false
}
