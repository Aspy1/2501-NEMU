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
    {"eax", 0}, {"ebx", 0}, {"ecx", 0},
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
        case OR: return 9;
        case AND: return 8;
        case EQ: case NEQ: return 4;
        case '+': case '-': return 2;
        case '*': case '/': return 1;
        case NOT: case NEG: case REF: return 0; // 一元操作符优先级最高
        default: return -1; // 非运算符
    }
}

// 比较两个运算符的优先级
static inline int op_prec_cmp(int t1, int t2) {
    return op_prec(t1) - op_prec(t2);
}

// 在区间[s, e]内找到主导运算符
static int find_dominated_op(int s, int e, bool *success) {
    int i;
    int bracket_level = 0;
    int dominated_op = -1;
    
    for(i = s; i <= e; i++) {
        switch(tokens[i].type) {
            case REG: case NUM: case HEX: break;
            
            case '(': 
                bracket_level++;
                break;
                
            case ')': 
                bracket_level--;
                if(bracket_level < 0) {
                    *success = false;
                    return -1;
                }
                break;
                
            default:
                if(bracket_level == 0) {
                    if(dominated_op == -1 || 
                       op_prec_cmp(tokens[dominated_op].type, tokens[i].type) < 0 ||
                       (op_prec_cmp(tokens[dominated_op].type, tokens[i].type) == 0 && 
                        tokens[i].type != NOT && tokens[i].type != NEG && tokens[i].type != REF)) {
                        dominated_op = i;
                    }
                }
                break;
        }
    }
    
    *success = (dominated_op != -1);
    return dominated_op;
}

// 递归计算表达式
static uint32_t eval(int s, int e, bool *success) {
    if(s > e) {
        *success = false;
        return 0;
    }
    else if(s == e) {
        // 单个token
        uint32_t val;
        switch(tokens[s].type) {
            case REG:
                val = get_register_value(tokens[s].str + 1); // 跳过'$'
                break;
                
            case NUM:
                val = atoi(tokens[s].str);
                break;
                
            case HEX:
                val = strtol(tokens[s].str, NULL, 16);
                break;
                
            default:
                *success = false;
                return 0;
        }
        *success = true;
        return val;
    }
    else if(tokens[s].type == '(' && tokens[e].type == ')') {
        // 括号表达式
        return eval(s + 1, e - 1, success);
    }
    else {
        // 找到主导运算符
        int dominated_op = find_dominated_op(s, e, success);
        if(!*success) return 0;
        
        int op_type = tokens[dominated_op].type;
        
        // 处理一元操作符
        if(op_type == NOT || op_type == NEG || op_type == REF) {
            uint32_t val = eval(dominated_op + 1, e, success);
            if(!*success) return 0;
            
            switch(op_type) {
                case NOT: return val == 0 ? 1 : 0;
                case NEG: return -val;
                case REF: 
                    // 解引用操作
                    return hwaddr_read(val, 4);
                default: 
                    *success = false;
                    return 0;
            }
        }
        
        // 处理二元操作符
        uint32_t left_val = eval(s, dominated_op - 1, success);
        if(!*success) return 0;
        
        uint32_t right_val = eval(dominated_op + 1, e, success);
        if(!*success) return 0;
        
        switch(op_type) {
            case '+': return left_val + right_val;
            case '-': return left_val - right_val;
            case '*': return left_val * right_val;
            case '/': 
                if(right_val == 0) {
                    printf("Division by zero\n");
                    *success = false;
                    return 0;
                }
                return left_val / right_val;
            case EQ: return left_val == right_val ? 1 : 0;
            case NEQ: return left_val != right_val ? 1 : 0;
            case AND: return (left_val && right_val) ? 1 : 0;
            case OR: return (left_val || right_val) ? 1 : 0;
            default: 
                *success = false;
                return 0;
        }
    }
}

uint32_t expr(char *e, bool *success) {
    if(!make_token(e)) {
        *success = false;
        return 0;
    }
    
    // 调试输出
    printf("Tokens (%d):\n", nr_token);
    for (int i = 0; i < nr_token; i++) {
        const char *type_str = "UNKNOWN";
        switch(tokens[i].type) {
            case NOTYPE: type_str = "NOTYPE"; break;
            case EQ: type_str = "EQ"; break;
            case NUM: type_str = "NUM"; break;
            case NEQ: type_str = "NEQ"; break;
            case OR: type_str = "OR"; break;
            case AND: type_str = "AND"; break;
            case NOT: type_str = "NOT"; break;
            case DEREF: type_str = "DEREF"; break;
            case HEX: type_str = "HEX"; break;
            case REG: type_str = "REG"; break;
            case '(': type_str = "("; break;
            case ')': type_str = ")"; break;
            case '+': type_str = "+"; break;
            case '-': type_str = "-"; break;
            case '*': type_str = "*"; break;
            case '/': type_str = "/"; break;
        }
        printf("[%d] type=%s, str=%s\n", i, type_str, tokens[i].str);
    }
    
    // 预处理：将某些运算符转换为一元运算符
    for(int i = 0; i < nr_token; i++) {
        if(tokens[i].type == '-') {
            if(i == 0) {
                tokens[i].type = NEG;
                continue;
            }
            
            int prev_type = tokens[i - 1].type;
            if(!(prev_type == ')' || prev_type == NUM || prev_type == HEX || prev_type == REG)) {
                tokens[i].type = NEG;
            }
        }
        else if(tokens[i].type == '*') {
            if(i == 0) {
                tokens[i].type = REF;
                continue;
            }
            
            int prev_type = tokens[i - 1].type;
            if(!(prev_type == ')' || prev_type == NUM || prev_type == HEX || prev_type == REG)) {
                tokens[i].type = REF;
            }
        }
    }
    
    *success = true;
    uint32_t result = eval(0, nr_token - 1, success);
    
    // 检查是否处理了所有 token
    if (!*success) {
        printf("Expression evaluation failed\n");
        return 0;
    }
    
    return result;
}
//p (!($ecx != 0x00008000) &&($eax ==0x00000000))+0x12345678
//p 0xc0100000-(($edx+0x1234-10)*16)/4