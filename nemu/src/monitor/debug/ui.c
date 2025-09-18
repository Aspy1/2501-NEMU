#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	//定义静态函数 cmd_c，参数为字符指针 args
	cpu_exec(-1);
	//调用 cpu_exec 函数，默认传入-1。
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_d(char *args);

static int cmd_w(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "The program pauses after single-stepping through N instructions. If N is not specified, it defaults to 1.",cmd_si},
	{ "info","Print register status[r] or watchpoint information[w]",cmd_info},
	{ "x","Examine memory at a given address",cmd_x},
	{ "p","Calculate the value of the expression EXPR.", cmd_p},
	{ "d","Delete the monitoring point by number",cmd_d},
	{ "w", "Set a watchpoint for an expression", cmd_w}
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

static int cmd_si(char *args) {
    char *arg = strtok(NULL, " ");
    int times = 1;
    if(arg != NULL) {
        times = atoi(arg);
        if(times <= 0) times = 1; // 防止非法输入
    }
    for(int i = 0; i < times; i++) {
        cpu_exec(1);
    }
    return 0;
}

static int cmd_info(char*args){
    char *arg = strtok(NULL," ");
    if(arg == NULL){
        printf("Please specify 'r' for registers or 'w' for watchpoints.\n");
        return 0;
    }
    else{
        if(strcmp(arg,"r")==0){
            //只打印四个寄存器，格式为 $寄存器名 (十六进制寄存器内容)
            printf("$eax (0x%08x)\n", cpu.gpr[0]._32);
            printf("$ecx (0x%08x)\n", cpu.gpr[1]._32);
            printf("$edx (0x%08x)\n", cpu.gpr[2]._32);
            printf("$ebx (0x%08x)\n", cpu.gpr[3]._32);
			//printf("$eip (0x%08x)\n", cpu.eip); // 打印eip
            return 0;
        }
        else if(strcmp(arg,"w")==0){
            //打印使用中的监视点信息
            WP* current = get_head_wp();  // 使用函数获取head指针
    		if(current == NULL) {
                printf("No watchpoints currently set.\n");
            } else {
                printf("Watchpoint list:\n");
                while(current != NULL) {
                    printf("Watchpoint %d: %s = %u (0x%x)\n", 
                           current->NO, current->expr, current->value, current->value);
                    current = current->next;
                }
            }
            return 0;
        }
        else{
            printf("Unknown argument '%s'. Please specify 'r' for registers or 'w' for watchpoints.\n",arg);
            return 0;
        }
	}
}

static int cmd_x(char *args) {
	char *arg1 = strtok(NULL, " ");
	char *arg2 = strtok(NULL, " ");
	//printf("arg1: %s, arg2: %s\n", arg1, arg2); // Debugging line
	if(arg1 == NULL || arg2 == NULL) {
		printf("Usage: x N EXPR\n");
		return 0;
	}
	int N = atoi(arg1);
	if (N <= 0) {
		printf("N should be a positive integer.\n");
		return 0;
	}
	else
	{
            //printf("0x%08x\n", cpu.eip + i );//打印N个内存地址，起始地址为eip. 作为测试
            //起始地址替换为arg2表达式的值
        swaddr_t base_addr = strtoul(arg2, NULL, 16);
        for(int i = 0; i < N; i++){
            swaddr_t addr = base_addr + i*4;
            uint32_t data = swaddr_read(addr, 4); // 读取4字节内容
            printf("0x%08x ", data);
            //打印从base_addr开始的N个4字节内容
        }
    }
	return 0;
}

static int cmd_p(char *args) {
	if (args == NULL || *args == '\0') {
        printf("Usage: p EXPRESSION\n");
        return 0;
    }
	bool success;
	uint32_t result = expr(args, &success);
	// expr 函数用于计算表达式的值，返回结果为 uint32_t 类型，并通过 success 指针返回计算是否成功
	// expr 函数在 expr.c 中实现
	if (success) {
		printf("%u\n", result); // 输出计算结果
	} else {
		printf("Invalid expression.\n");
	}
	return 0;
}

static int cmd_d(char *args) {
    if (args == NULL || *args == '\0') {
        printf("Usage: d WP_NUM\n");
        return 0;
    }
    
    int wp_num = atoi(args);
    // 这里需要遍历监视点链表找到对应编号的监视点并删除
    WP* current = get_head_wp();
    WP* prev = NULL;  
    
    while (current != NULL) {
        if (current->NO == wp_num) {
            // 找到要删除的监视点
            if (prev == NULL) {
                // 要删除的是头节点
            } else {
                prev->next = current->next;
            }
            free_wp(current);
            printf("Deleted watchpoint %d\n", wp_num);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    printf("Watchpoint %d not found\n", wp_num);
    return 0;
}

static int cmd_w(char *args) {
    if (args == NULL || *args == '\0') {
        printf("Usage: w EXPRESSION\n");
        return 0;
    }
    
    WP* wp = create_wp(args);
    if (wp == NULL) {
        printf("Failed to create watchpoint for expression: %s\n", args);
        return 0;
    }
    
    printf("Watchpoint %d created for expression: %s\n", wp->NO, wp->expr);
    return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
