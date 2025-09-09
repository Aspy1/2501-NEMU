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
	int ret;
	const int ramdisk_max_size = 0xa0000;
	FILE *fp = fopen(exec_file, "rb");
	Assert(fp, "Can not open '%s'", exec_file);

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	Assert(file_size < ramdisk_max_size, "file size(%zd) too large", file_size);

	fseek(fp, 0, SEEK_SET);
	ret = fread(hwa_to_va(0), file_size, 1, fp);
	assert(ret == 1);
	fclose(fp);
}
#endif

static void load_entry() {
	int ret;
	FILE *fp = fopen("entry", "rb");
	Assert(fp, "Can not open 'entry'");

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);

	fseek(fp, 0, SEEK_SET);
	ret = fread(hwa_to_va(ENTRY_START), file_size, 1, fp);
	assert(ret == 1);
	fclose(fp);
}

void restart() {
	/* Perform some initialization to restart a program */
#ifdef USE_RAMDISK
	/* Read the file with name `argv[1]' into ramdisk. */
	init_ramdisk();
#endif

	/* Read the entry code into memory. */
	load_entry();

	/* Set the initial instruction pointer. */
	cpu.eip = ENTRY_START;

	/* Initialize DRAM. */
	init_ddr3();
}
