#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
//翻译：我们使用 POSIX 正则表达式函数来处理正则表达式
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum {
	NOTYPE = 256, EQ = 257, NUM = 258,
	NEQ = 259,    // 不等于 !=
	OR = 260,     // 逻辑或 ||
	AND = 261,    // 逻辑与 &&
	NOT = 262,    // 逻辑非 !
	DEREF = 263,  // 指针解引用 *
	HEX = 264,    // 十六进制数
	REG = 265     // 寄存器

	/* TODO: Add more token types */

};// 定义 token 类型枚举

//NOTYPE从256开始的原因是 ASCII 码中 0-255 已经被占用
static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */
	//题目翻译：注意不同规则的优先级

	{" ", NOTYPE},				// spaces
	{"\\|\\|", OR},				// 逻辑或 || 
	{"&&", AND},				// 逻辑与 &&
	{"!=", NEQ},				// 不等于 !=
	{"\\+", '+'},					// plus
	//解释：\\表示斜杠，而表示+是斜杠+，所以\\+表示加号本身
	//这也说明了\\转义的优先级更高
	{"==", EQ},						// equal
	{"\\*",'*'},					// multiply
	{"\\/",'/'},		
	{"-",'-'},
	{"\\(",'('},
	{"\\)",')'},
	{"!", NOT},					// 逻辑非 !
	//等号并没有特殊含义，因此不需要加反斜杠
	{"0x[0-9a-fA-F]+", HEX},	// 十六进制数
	{"\\$[a-zA-Z0-9_]+", REG},	// 寄存器 
	{"[0-9]+", NUM},

};
// 通常情况下 忽略所有的空格，优先级：小括号 高于乘除 高于加减 
// 转义字符\\表示一个斜杠
// 正则表达式中，+ * () / 都是有特殊含义的字符，如果要表示它们本身，需要在前面加上反斜杠\进行转义 比如\\+表示加号本身

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

typedef struct {
	char name[16];
	uint32_t value;
} Register;

Register registers[] = {
	{"eax", 0},
	{"ebx", 0},
	{"ecx", 0},
	{"edx", 0},
	{"esp", 0},
	{"ebp", 0},
	{"eip", 0},
};

#define NUM_REGISTERS (sizeof(registers) / sizeof(registers[0]))

uint32_t get_register_value(const char *name) {
	for (int i = 0; i < NUM_REGISTERS; i++) {
		if (strcmp(registers[i].name, name) == 0) {
			return registers[i].value;
		}
	}
	printf("Unknown register: %s\n", name);
	return 0;
}// 获取寄存器值


static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}
// 初始化所有正则表达式，只编译一次

typedef struct token {
	int type;
	char str[32];
} Token;
//定义一个结构体类型 Token，包含两个成员：type 和 str



Token tokens[32];
int nr_token;  //记录识别出的token的数量

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {  //while执行的条件是e[position]不为字符串结束符，其中e是传入的字符串，position是字符串的索引
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			//对NR_REGEX的定义是 规则的数目(规则数组的大小/单个规则的大小)
			//也就是说，这里对每个位置的token，都要尝试所有的规则
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				//逐步分析if的成立条件：
				//regexec 函数用于在字符串中查找是否有符合指定正则表达式的子串。
				//int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);
				//汉语表述大致为：在字符串string中查找是否有符合正则表达式preg的子串，找到后将匹配结果存储在pmatch数组中，nmatch表示pmatch数组的大小，eflags用于控制匹配行为，默认为0
				//返回值为0表示成功匹配，返回非0值表示没有匹配成功或发生错误
				//这里的preg是&re[i]，string是e + position，nmatch是1，pmatch是&pmatch，eflags是0
				//e + position表示从字符串e的position位置开始匹配
				//pmatch是一个结构体，包含两个成员：rm_so和rm_eo，分别表示匹配子串在字符串中的起始和结束位置的偏移量
				//pmatch.rm_so == 0 的意思是匹配的子串必须从当前位置开始
				//综上所述，这个if条件的意思是：如果在字符串e从position位置开始，有符合规则re[i]的子串，并且这个子串从position位置开始
				//如果if条件成立，说明在当前位置找到了一个符合规则的token

				//printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s\n", i, rules[i].regex, position, pmatch.rm_eo - pmatch.rm_so, pmatch.rm_eo - pmatch.rm_so, e + position);
				//debug

				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;
				//综上所述 这里实现了一个正确的token识别 我不想看了 看不懂

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */
				//翻译：现在使用 rules[i] 识别了一个新令牌。添加代码以在数组 `tokens` 中记录令牌。对于某些类型的令牌，应执行一些额外的操作。

				switch(rules[i].token_type) { //switch的格式：switch(令牌类型) { case: ...
					//每种情况，向tokens数组添加类型名称 并记录这一子串。记录完成后，指针后移。
					case NOTYPE: 
					break; //忽略空格 但是要更新position
					case NUM: {
                        if (nr_token >= 32) {
                            printf("too many tokens\n");
                            return false;
                        }
                        tokens[nr_token].type = NUM;
                        // 复制数字字符串，确保不超过缓冲区大小
                        int len = substr_len < 31 ? substr_len : 31;
                        strncpy(tokens[nr_token].str, substr_start, len);
                        tokens[nr_token].str[len] = '\0';
                        nr_token++;
                        break;
                    }
					case HEX: {
                        if (nr_token >= 32) {
                            printf("too many tokens\n");
                            return false;
                        }
                        tokens[nr_token].type = HEX; // 正确设置十六进制类型
                        int hex_len = substr_len < 31 ? substr_len : 31;
                        strncpy(tokens[nr_token].str, substr_start, hex_len);
                        tokens[nr_token].str[hex_len] = '\0';
                        nr_token++;
                        break;
                    }
					case REG: {
                        if (nr_token >= 32) {
                            printf("too many tokens\n");
                            return false;
                        }
                        tokens[nr_token].type = REG;
                        int reg_len = substr_len < 31 ? substr_len : 31;
                        strncpy(tokens[nr_token].str, substr_start, reg_len);
                        tokens[nr_token].str[reg_len] = '\0';
                        nr_token++;
                        break;
                    }
						case '+': case '-': case '*': case '/': case EQ: case '(': case ')': case NEQ: case OR: case AND: case NOT:
                        if (nr_token >= 32) {
                            printf("too many tokens\n");
                            return false;
                        }
                        tokens[nr_token].type = rules[i].token_type;
                        tokens[nr_token].str[0] = '\0'; // 运算符和括号无需保存字符串
                        nr_token++;
                        break;


					default: panic("please implement me"); //default的作用是 如果上面所有的case都不符合，就执行default后的语句 panic函数格式为 panic("错误信息") ，打印错误信息并终止程序
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

// 获取当前 token
// 递归下降分析相关变量和函数
static int pos = 0;// 当前分析到的 token 下标

// 声明所有表达式求值函数
uint32_t eval_expr();
uint32_t eval_and();
uint32_t eval_eq_neq();
uint32_t eval_add_sub();
uint32_t eval_term();
uint32_t eval_factor();

// 解析因子（数字或括号表达式）

uint32_t eval_factor() {
    // 处理一元操作符：逻辑非!和解引用*
    if (tokens[pos].type == NOT || tokens[pos].type == '*') {
        int op = tokens[pos].type;
        pos++;
        
        // 直接解析下一个因子，不再特殊处理括号
        uint32_t operand = eval_factor();
        
        // 应用一元操作
        if (op == NOT) {
            // 显式检查操作数是否为0
            return (operand == 0) ? 1 : 0;
        } else if (op == '*') {
            // 解引用操作
            return *((uint32_t*)operand);
        }
    }
    
    // 处理数字
    if (tokens[pos].type == NUM) {
        uint32_t val = atoi(tokens[pos].str);
        pos++;
        return val;
    }
    
    // 处理十六进制数
    if (tokens[pos].type == HEX) {
        // 跳过 "0x" 前缀
        const char *hex_str = tokens[pos].str;
        if (strncmp(hex_str, "0x", 2) == 0) {
            hex_str += 2;
        }
        uint32_t val = strtol(hex_str, NULL, 16);
        pos++;
        return val;
    }
    
    // 处理寄存器
    if (tokens[pos].type == REG) {
        // 跳过$符号，获取寄存器名称
        const char *reg_name = tokens[pos].str + 1;
        uint32_t val = get_register_value(reg_name);
        pos++;
        return val;
    }
    
    // 处理括号表达式
    if (tokens[pos].type == '(') {
        pos++;
        uint32_t val = eval_expr();
        if (pos < nr_token && tokens[pos].type == ')') {
            pos++;
        } else {
            printf("Missing closing parenthesis\n");
            exit(1);
        }
        return val;
    }
    
    printf("Invalid factor at position %d: type=%d\n", pos, tokens[pos].type);
    exit(1);
    return 0;
}
uint32_t eval_term() {
    uint32_t val = eval_factor();
    
    while (pos < nr_token && (tokens[pos].type == '*' || tokens[pos].type == '/')) {
        int op = tokens[pos].type;
        pos++;
        uint32_t rhs = eval_factor();
        
        if (op == '*') {
            val *= rhs;
        } else if (op == '/') {
            if (rhs == 0) {
                printf("Division by zero\n");
                exit(1);
            }
            val /= rhs;
        }
    }
    return val;
}

uint32_t eval_add_sub() {
    // 处理可能的负号（一元减号）
    if (tokens[pos].type == '-') {
        pos++;
        return -eval_term(); // 一元减号
    }
    
    uint32_t val = eval_term();
    
    while (pos < nr_token && (tokens[pos].type == '+' || tokens[pos].type == '-')) {
        int op = tokens[pos].type;
        pos++;
        uint32_t rhs = eval_term();
        
        if (op == '+') {
            val += rhs;
        } else if (op == '-') {
            val -= rhs;
        }
    }
    return val;
}

uint32_t eval_eq_neq() {
    uint32_t val = eval_add_sub();
    
    while (pos < nr_token && (tokens[pos].type == EQ || tokens[pos].type == NEQ)) {
        int op = tokens[pos].type;
        pos++;
        uint32_t rhs = eval_add_sub();
        
        if (op == EQ) {
            val = (val == rhs) ? 1 : 0;
        } else { // NEQ
            val = (val != rhs) ? 1 : 0;
        }
    }
    return val;
}

uint32_t eval_and() {
    uint32_t val = eval_eq_neq();
    
    while (pos < nr_token && tokens[pos].type == AND) {
        pos++;
        uint32_t rhs = eval_eq_neq();
        val = (val && rhs) ? 1 : 0;
    }
    return val;
}

uint32_t eval_expr() {
    uint32_t val = eval_and();
    
    while (pos < nr_token && tokens[pos].type == OR) {
        pos++;
        uint32_t rhs = eval_and();
        val = (val || rhs) ? 1 : 0;
    }
    return val;
}

uint32_t expr(char *e, bool *success) {
    if(!make_token(e)) {
        *success = false;
        return 0;
    }
    
    // 调试输出：打印所有识别出的 tokens
    printf("Tokens:\n");
    for (int i = 0; i < nr_token; i++) {
       printf("[%d] type=%d, str=%s\n", i, tokens[i].type, tokens[i].str);
    }
    
    *success = true;
    pos = 0;
    
    uint32_t result = eval_expr();
    
    // 检查是否处理了所有 token
    if (pos < nr_token) {
        printf("Unexpected token at position %d: type=%d, str=%s\n", 
               pos, tokens[pos].type, tokens[pos].str);
        *success = false;
        return 0;
    }
    
    return result;
}
//p (!($ecx != 0x00008000) &&($eax ==0x00000000))+0x12345678