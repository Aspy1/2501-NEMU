#include "common.h"
#include <stdlib.h>
#include <elf.h>

char *exec_file = NULL;

static char *strtab = NULL;
static Elf32_Sym *symtab = NULL;
static int nr_symtab_entry;

void load_elf_tables(int argc, char *argv[]) { //定义void类型的函数 load_elf_tables，参数为 int 类型的 argc 和 char* 类型的 argv[] 数组
	int ret; //定义 int 类型的变量 ret 用于存储函数调用的返回值
	Assert(argc == 2, "run NEMU with format 'nemu [program]'"); 
	//调用assert函数，检查 argc 是否等于 2，如果不等于 2 则输出错误信息并终止程序运行
	//错误信息的翻译是 "以 'nemu [program]' 格式运行 NEMU" 

	exec_file = argv[1];
	//argc/argv：argc 和 argv 是 main 函数的参数，用于处理​​命令行参数；它们允许程序在启动时接收用户输入的额外信息。
	//argc：表示命令行参数的数量，包括程序本身的名称，也就是说 argc 至少为 1。类型：int
	//argv：是一个字符串数组，包含了所有的命令行参数。argv[0] 通常是程序的名称；argv[argc]固定为NULL，标志数组结束。其余参数依次存储在 argv[1] 到 argv[argc-1] 中，均为用户输入的参数。类型：char*[]
	//这一步将用户提供的第一个参数赋值给全局变量 exec_file。 
	//对“用户提供的第一个参数”举例：如果用户在命令行输入 "nemu my_program"，那么 exec_file 将被赋值为 "my_program"。

	FILE *fp = fopen(exec_file, "rb");
	//定义文件指针 fp，并使用 fopen 函数以二进制读模式 ("rb") 打开由 exec_file 指定的文件
	//“rb模式”：以二进制模式打开文件进行读取 也就是打开argv[1]指定的文件

	Assert(fp, "Can not open '%s'", exec_file);
	//调用 assert 函数，检查文件指针 fp 是否为 NULL，如果为 NULL 则表示文件打开失败，输出错误信息并终止程序运行

	uint8_t buf[sizeof(Elf32_Ehdr)]; 
	//uint8_t 是 C 语言中的一种数据类型，表示无符号的 8 位整数，取值范围为 0 到 255
	//定义一个名为 buf 的数组，数组大小为 Elf32_Ehdr 结构体的大小。Elf32_Ehdr 结构体定义了 ELF 文件的头部信息，包含了文件的基本属性和布局

	ret = fread(buf, sizeof(Elf32_Ehdr), 1, fp);
	//调用fread函数对返回值进行赋值 此处函数（右侧）的作用：将 1个（参数3） Elf32_Ehdr 大小的数据（参数2）从文件指针 fp（参数4）指向的文件中读取，并存储到 buf（参数1）数组中 
	//fread函数的返回值表示实际读取的元素数量 在此处应该是 1

	assert(ret == 1); 
	//调用 assert 函数，检查 ret 是否等于 1，如果不等于 1 则表示读取失败，程序会输出错误信息并终止运行
	//ret不为1代表fread函数读取的 Elf32_Ehdr 结构体 不完整，输出错误信息。

	/* The first several bytes contain the ELF header. */
	//翻译：前几个字节包含 ELF 头部信息 
	Elf32_Ehdr *elf = (void *)buf;
	//将 buf 数组的地址强制转换为 Elf32_Ehdr 结构体指针，并赋值给 elf 变量 
	char magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

	/* Check ELF header */
	assert(memcmp(elf->e_ident, magic, 4) == 0);		// magic number
	assert(elf->e_ident[EI_CLASS] == ELFCLASS32);		// 32-bit architecture
	assert(elf->e_ident[EI_DATA] == ELFDATA2LSB);		// littel-endian
	assert(elf->e_ident[EI_VERSION] == EV_CURRENT);		// current version
	assert(elf->e_ident[EI_OSABI] == ELFOSABI_SYSV || 	// UNIX System V ABI
			elf->e_ident[EI_OSABI] == ELFOSABI_LINUX); 	// UNIX - GNU
	assert(elf->e_ident[EI_ABIVERSION] == 0);			// should be 0
	assert(elf->e_type == ET_EXEC);						// executable file
	assert(elf->e_machine == EM_386);					// Intel 80386 architecture
	assert(elf->e_version == EV_CURRENT);				// current version


	/* Load symbol table and string table for future use */

	/* Load section header table */
	uint32_t sh_size = elf->e_shentsize * elf->e_shnum;
	Elf32_Shdr *sh = malloc(sh_size);
	fseek(fp, elf->e_shoff, SEEK_SET);
	ret = fread(sh, sh_size, 1, fp);
	assert(ret == 1);

	/* Load section header string table */
	char *shstrtab = malloc(sh[elf->e_shstrndx].sh_size);
	fseek(fp, sh[elf->e_shstrndx].sh_offset, SEEK_SET);
	ret = fread(shstrtab, sh[elf->e_shstrndx].sh_size, 1, fp);
	assert(ret == 1);

	int i;
	for(i = 0; i < elf->e_shnum; i ++) {
		if(sh[i].sh_type == SHT_SYMTAB && 
				strcmp(shstrtab + sh[i].sh_name, ".symtab") == 0) {
			/* Load symbol table from exec_file */
			symtab = malloc(sh[i].sh_size);
			fseek(fp, sh[i].sh_offset, SEEK_SET);
			ret = fread(symtab, sh[i].sh_size, 1, fp);
			assert(ret == 1);
			nr_symtab_entry = sh[i].sh_size / sizeof(symtab[0]);
		}
		else if(sh[i].sh_type == SHT_STRTAB && 
				strcmp(shstrtab + sh[i].sh_name, ".strtab") == 0) {
			/* Load string table from exec_file */
			strtab = malloc(sh[i].sh_size);
			fseek(fp, sh[i].sh_offset, SEEK_SET);
			ret = fread(strtab, sh[i].sh_size, 1, fp);
			assert(ret == 1);
		}
	}

	free(sh);
	free(shstrtab);

	assert(strtab != NULL && symtab != NULL);

	fclose(fp);
}

