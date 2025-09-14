#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
//翻译：我们使用 POSIX 正则表达式函数来处理正则表达式
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ = 257, NUM = 258

	/* TODO: Add more token types */

};
//enum 的作用是 定义一组命名的整数常量
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
	{"\\+", '+'},					// plus
	//解释：\\表示斜杠，而表示+是斜杠+，所以\\+表示加号本身
	//这也说明了\\转义的优先级更高
	{"==", EQ},						// equal
	{"\\*",'*'},					// multiply
	{"\\/",'/'},		
	{"-",'-'},
	{"\\(",'('},
	{"\\)",')'},
	//等号并没有特殊含义，因此不需要加反斜杠
	//接下来判断十进制整数
	{"[0-9]+", NUM},

};
// 通常情况下 忽略所有的空格，优先级：小括号 高于乘除 高于加减 
// 转义字符\\表示一个斜杠
// 正则表达式中，+ * () / 都是有特殊含义的字符，如果要表示它们本身，需要在前面加上反斜杠\进行转义 比如\\+表示加号本身
#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

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

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {  //while执行的条件是e[position]不为字符串结束符，其中e是传入的字符串，position是字符串的索引
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */
				//翻译：现在使用 rules[i] 识别了一个新令牌。添加代码以在数组 `tokens` 中记录令牌。对于某些类型的令牌，应执行一些额外的操作。

				switch(rules[i].token_type) { //switch的格式：switch(令牌类型) { case: ...
					//每种情况，向tokens数组添加类型名称 并记录这一子串。记录完成后，指针后移。
					case NOTYPE: break; //忽略空格

					default: panic("please implement me");
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

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	panic("please implement me");
	return 0;
}

