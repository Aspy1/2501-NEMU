#include "nemu.h"
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum {
    NOTYPE = 256, EQ = 257, NUM = 258,
    NEQ = 259, OR = 260, AND = 261, 
    NOT = 262, DEREF = 263, HEX = 264, REG = 265,
    NEG = 266, REF = 267
};

static struct rule {
    char *regex;
    int token_type;
} rules[] = {
    {" ", NOTYPE}, {"\\|\\|", OR}, {"&&", AND}, 
    {"!=", NEQ}, {"\\+", '+'}, {"==", EQ}, 
    {"\\*",'*'}, {"\\/",'/'}, {"-",'-'},
    {"\\(",'('}, {"\\)",')'}, {"!", NOT},
    {"0x[0-9a-fA-F]+", HEX}, {"\\$[a-zA-Z0-9_]+", REG}, 
    {"[0-9]+", NUM}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))
static regex_t re[NR_REGEX];

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

    while(e[position] != '\0') {
        for(i = 0; i < NR_REGEX; i ++) {
            if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;
                position += substr_len;

                switch(rules[i].token_type) {
                    case NOTYPE: break;
                    case NUM:
                    case HEX:
                    case REG:
                        if (nr_token >= 32) return false;
                        tokens[nr_token].type = rules[i].token_type;
                        int len = substr_len < 31 ? substr_len : 31;
                        strncpy(tokens[nr_token].str, substr_start, len);
                        tokens[nr_token].str[len] = '\0';
                        nr_token++;
                        break;
                    default:
                        if (nr_token >= 32) return false;
                        tokens[nr_token].type = rules[i].token_type;
                        tokens[nr_token].str[0] = '\0';
                        nr_token++;
                        break;
                }
                break;
            }
        }

        if(i == NR_REGEX) {
            return false;
        }
    }

    return true; 
}

// 定义 Register 结构体
typedef struct {
    const char *name;
    uint32_t value;
} Register;

Register registers[] = {
    {"eax", 0}, {"ebx", 0}, {"ecx", 0x00008000},
    {"edx", 0}, {"esp", 0}, {"ebp", 0}, {"eip", 0}
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
}

// 获取运算符优先级
int op_prec(int token_type) {
    switch(token_type) {
        case OR: return 5;
        case AND: return 4;
        case EQ: case NEQ: return 3;
        case '+': case '-': return 2;
        case '*': case '/': return 1;
        case NOT: case NEG: case REF: return 0; // 一元操作符优先级最高
        default: return -1; // 非运算符
    }
}

// // 打印当前表达式区间
// void print_expression_interval(int s, int e) {
//     printf("当前表达式区间: [");
//     for (int i = s; i <= e; i++) {
//         if (i > s) printf(" ");
//         switch(tokens[i].type) {
//             case REG: case HEX: case NUM: 
//                 printf("%s", tokens[i].str);
//                 break;
//             case '(': printf("("); break;
//             case ')': printf(")"); break;
//             case EQ: printf("=="); break;
//             case NEQ: printf("!="); break;
//             case OR: printf("||"); break;
//             case AND: printf("&&"); break;
//             case NOT: printf("!"); break;
//             case NEG: printf("-"); break;
//             case REF: printf("*"); break;
//             case '+': printf("+"); break;
//             case '-': printf("-"); break;
//             case '*': printf("*"); break;
//             case '/': printf("/"); break;
//             default: printf("?");
//         }
//     }
//     printf("]\n");
// }

// 在区间[s, e]内找到主导运算符
static int find_dominated_op(int s, int e, bool *success) {
    int i;
    int bracket_level = 0;
    int dominated_op = -1;
    int min_prec = -1; // 最低优先级（数值最大）
    
    // printf("在区间 [%d, %d] 内查找主导运算符\n", s, e);
    // print_expression_interval(s, e);
    
    for(i = s; i <= e; i++) {
        switch(tokens[i].type) {
            case '(': 
                bracket_level++;
                // printf("遇到 '('，括号层级增加到 %d\n", bracket_level);
                break;
                
            case ')': 
                bracket_level--;
                // printf("遇到 ')'，括号层级减少到 %d\n", bracket_level);
                if(bracket_level < 0) {
                    *success = false;
                    return -1;
                }
                break;
                
            default:
                if(bracket_level == 0) {
                    int prec = op_prec(tokens[i].type);
                    if (prec >= 0) { // 是运算符
                        // printf("找到运算符 %s (优先级 %d)\n", tokens[i].str, prec);
                        
                        // 选择优先级最低的运算符（数值最大）
                        // 如果有多个相同优先级的，选择最右边的（左结合）
                        if (dominated_op == -1 || prec > min_prec || 
                            (prec == min_prec && i > dominated_op)) {
                            dominated_op = i;
                            min_prec = prec;
                            // printf("更新主导运算符为位置 %d: %s\n", i, tokens[i].str);
                        }
                    }
                }
                break;
        }
    }
    
    if (dominated_op != -1) {
        // printf("确定主导运算符为位置 %d: %s\n", dominated_op, tokens[dominated_op].str);
        *success = true;
    } else {
        // printf("未找到主导运算符\n");
        *success = false;
    }
    
    return dominated_op;
}

// 递归计算表达式
static uint32_t eval(int s, int e, bool *success) {
    // printf("\n开始计算表达式: ");
    // print_expression_interval(s, e);
    
    if(s > e) {
        printf("错误: s > e\n");
        *success = false;
        return 0;
    }
    else if(s == e) {
        // 单个token
        uint32_t val;
        switch(tokens[s].type) {
            case REG:
                val = get_register_value(tokens[s].str + 1); // 跳过'$'
                // printf("解析寄存器 %s = 0x%08x\n", tokens[s].str, val);
                break;
                
            case NUM:
                val = atoi(tokens[s].str);
                // printf("解析数字 %s = %d\n", tokens[s].str, val);
                break;
                
            case HEX:
                val = strtol(tokens[s].str, NULL, 16);
                // printf("解析十六进制 %s = 0x%08x\n", tokens[s].str, val);
                break;
                
            default:
                // printf("错误: 未知的单个token类型 %d\n", tokens[s].type);
                *success = false;
                return 0;
        }
        *success = true;
        return val;
    }
    else if(tokens[s].type == '(' && tokens[e].type == ')') {
        // 括号表达式
        // printf("去掉外层括号\n");
        return eval(s + 1, e - 1, success);
    }
    else {
        // 找到主导运算符
        int dominated_op = find_dominated_op(s, e, success);
        if(!*success) {
            // printf("错误: 未找到主导运算符\n");
            return 0;
        }
        
        int op_type = tokens[dominated_op].type;
        // printf("主导运算符类型: %d\n", op_type);
        
        // 处理一元操作符
        if(op_type == NOT || op_type == NEG || op_type == REF) {
            // printf("处理一元操作符 %s\n", tokens[dominated_op].str);
            uint32_t val = eval(dominated_op + 1, e, success);
            if(!*success) return 0;
            
            uint32_t result;
            switch(op_type) {
                case NOT: 
                    result = (val == 0) ? 1 : 0;
                    // printf("!%d = %d\n", val, result);
                    break;
                case NEG: 
                    result = -val;
                    // printf("-%d = %d\n", val, result);
                    break;
                case REF: 
                    // 解引用操作
                    // printf("*0x%08x\n", val);
                    result = hwaddr_read(val, 4);
                    break;
                default: 
                    *success = false;
                    return 0;
            }
            return result;
        }
        
        // 处理二元操作符
        // printf("处理二元操作符 %s\n", tokens[dominated_op].str);
        // printf("左操作数区间: [%d, %d]\n", s, dominated_op - 1);
        uint32_t left_val = eval(s, dominated_op - 1, success);
        if(!*success) return 0;
        
        // printf("右操作数区间: [%d, %d]\n", dominated_op + 1, e);
        uint32_t right_val = eval(dominated_op + 1, e, success);
        if(!*success) return 0;
        
        uint32_t result;
        switch(op_type) {
            case '+': 
                result = left_val + right_val;
                // printf("%d + %d = %d\n", left_val, right_val, result);
                break;
            case '-': 
                result = left_val - right_val;
                // printf("%d - %d = %d\n", left_val, right_val, result);
                break;
            case '*': 
                result = left_val * right_val;
                // printf("%d * %d = %d\n", left_val, right_val, result);
                break;
            case '/': 
                if(right_val == 0) {
                    // printf("错误: 除以零\n");
                    *success = false;
                    return 0;
                }
                result = left_val / right_val;
                // printf("%d / %d = %d\n", left_val, right_val, result);
                break;
            case EQ: 
                result = (left_val == right_val) ? 1 : 0;
                // printf("%d == %d = %d\n", left_val, right_val, result);
                break;
            case NEQ: 
                result = (left_val != right_val) ? 1 : 0;
                // printf("%d != %d = %d\n", left_val, right_val, result);
                break;
            case AND: 
                result = (left_val && right_val) ? 1 : 0;
                // printf("%d && %d = %d\n", left_val, right_val, result);
                break;
            case OR: 
                result = (left_val || right_val) ? 1 : 0;
                //printf("%d || %d = %d\n", left_val, right_val, result);
                break;
            default: 
                // printf("错误: 未知的二元操作符类型 %d\n", op_type);
                *success = false;
                return 0;
        }
        return result;
    }
}

uint32_t expr(char *e, bool *success) {
    if(!make_token(e)) {
        *success = false;
        return 0;
    }
    
    // // 调试输出
    // printf("Tokens (%d):\n", nr_token);
    // for (int i = 0; i < nr_token; i++) {
    //     const char *type_str = "UNKNOWN";
    //     switch(tokens[i].type) {
    //         case NOTYPE: type_str = "NOTYPE"; break;
    //         case EQ: type_str = "EQ"; break;
    //         case NUM: type_str = "NUM"; break;
    //         case NEQ: type_str = "NEQ"; break;
    //         case OR: type_str = "OR"; break;
    //         case AND: type_str = "AND"; break;
    //         case NOT: type_str = "NOT"; break;
    //         case DEREF: type_str = "DEREF"; break;
    //         case HEX: type_str = "HEX"; break;
    //         case REG: type_str = "REG"; break;
    //         case '(': type_str = "("; break;
    //         case ')': type_str = ")"; break;
    //         case '+': type_str = "+"; break;
    //         case '-': type_str = "-"; break;
    //         case '*': type_str = "*"; break;
    //         case '/': type_str = "/"; break;
    //         case NEG: type_str = "NEG"; break;
    //         case REF: type_str = "REF"; break;
    //     }
    //     printf("[%d] type=%s, str=%s\n", i, type_str, tokens[i].str);
    // }
    
    // 预处理：将某些运算符转换为一元运算符
    for(int i = 0; i < nr_token; i++) {
        if(tokens[i].type == '-') {
            if(i == 0) {
                tokens[i].type = NEG;
                // printf("转换 - 为一元负号 (NEG)\n");
                continue;
            }
            
            int prev_type = tokens[i - 1].type;
            if(!(prev_type == ')' || prev_type == NUM || prev_type == HEX || prev_type == REG)) {
                tokens[i].type = NEG;
                // printf("转换 - 为一元负号 (NEG)\n");
            }
        }
        else if(tokens[i].type == '*') {
            if(i == 0) {
                tokens[i].type = REF;
                // printf("转换 * 为一元解引用 (REF)\n");
                continue;
            }
            
            int prev_type = tokens[i - 1].type;
            if(!(prev_type == ')' || prev_type == NUM || prev_type == HEX || prev_type == REG)) {
                tokens[i].type = REF;
                // printf("转换 * 为一元解引用 (REF)\n");
            }
        }
    }
    
    *success = true;
	//    printf("\n开始表达式求值\n");
    uint32_t result = eval(0, nr_token - 1, success);
    
    if (*success) {
    //    printf("\n表达式求值成功，结果 = 0x%08x (%d)\n", result, result);
    } else {
    //    printf("\n表达式求值失败\n");
    }
    
    return result;
}
//p (!($ecx != 0x00008000) &&($eax ==0x00000000))+0x12345678
//p 0xc0100000-(($edx+0x1234-10)*16)/4
//标记任务5完成