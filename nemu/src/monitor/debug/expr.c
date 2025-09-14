#include "nemu.h"
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum {
    NOTYPE = 256, EQ = 257, NUM = 258,
    NEQ = 259, OR = 260, AND = 261, 
    NOT = 262, DEREF = 263, HEX = 264, REG = 265
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
static int pos = 0;

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

// 函数原型声明
uint32_t parse_expression(int min_priority);
uint32_t parse_factor();
uint32_t handle_unary_operator(int op, uint32_t operand);
uint32_t handle_binary_operator(int op, uint32_t left, uint32_t right);

// 获取运算符优先级
int get_priority(int token_type) {
    switch(token_type) {
        case OR: return 1;
        case AND: return 2;
        case EQ: case NEQ: return 3;
        case '+': case '-': return 4;
        case '*': case '/': return 5;
        case NOT: case DEREF: return 6; // 一元操作符优先级最高
        default: return 0;
    }
}

// 处理一元操作符
uint32_t handle_unary_operator(int op, uint32_t operand) {
    switch(op) {
        case NOT: return operand == 0 ? 1 : 0;
        case '*': 
            // 解引用操作 - 暂时不支持
            printf("Dereference operator * is not implemented\n");
            exit(1);
        case '-': return -operand;
        default: return operand;
    }
}

// 处理二元操作符
uint32_t handle_binary_operator(int op, uint32_t left, uint32_t right) {
    switch(op) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': 
            if (right == 0) {
                printf("Division by zero\n");
                exit(1);
            }
            return left / right;
        case EQ: return left == right ? 1 : 0;
        case NEQ: return left != right ? 1 : 0;
        case AND: return (left && right) ? 1 : 0;
        case OR: return (left || right) ? 1 : 0;
        default: return left;
    }
}

// 解析因子：数字、寄存器、括号表达式
uint32_t parse_factor() {
    if (tokens[pos].type == '(') {
        pos++;
        uint32_t val = parse_expression(0);
        if (pos < nr_token && tokens[pos].type == ')') {
            pos++;
            return val;
        } else {
            printf("Missing closing parenthesis\n");
            exit(1);
        }
    }
    
    if (tokens[pos].type == NUM) {
        uint32_t val = atoi(tokens[pos].str);
        pos++;
        return val;
    }
    
    if (tokens[pos].type == HEX) {
        const char *hex_str = tokens[pos].str;
        if (strncmp(hex_str, "0x", 2) == 0) {
            hex_str += 2;
        }
        uint32_t val = strtol(hex_str, NULL, 16);
        pos++;
        return val;
    }
    
    if (tokens[pos].type == REG) {
        const char *reg_name = tokens[pos].str + 1;
        uint32_t val = get_register_value(reg_name);
        pos++;
        return val;
    }
    
    printf("Invalid factor at position %d: type=%d\n", pos, tokens[pos].type);
    exit(1);
    return 0;
}

// 主解析函数
uint32_t parse_expression(int min_priority) {
    uint32_t left;
    
    // 处理一元操作符
    if (pos < nr_token && 
        (tokens[pos].type == NOT || tokens[pos].type == '*' || tokens[pos].type == '-')) {
        int op = tokens[pos].type;
        pos++;
        uint32_t operand = parse_expression(6); // 一元操作符优先级最高
        left = handle_unary_operator(op, operand);
    } else {
        left = parse_factor();
    }
    
    // 处理二元操作符
    while (pos < nr_token) {
        int token_type = tokens[pos].type;
        int priority = get_priority(token_type);
        
        // 如果优先级低于最小值或遇到右括号，停止解析
        if (priority < min_priority || token_type == ')') {
            break;
        }
        
        pos++;
        uint32_t right = parse_expression(priority);
        left = handle_binary_operator(token_type, left, right);
    }
    
    return left;
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
    
    *success = true;
    pos = 0;
    
    uint32_t result = parse_expression(0);
    
    // 检查是否处理了所有 token
    if (pos < nr_token) {
        printf("Unexpected token at position %d: type=%d, str=%s\n", 
               pos, tokens[pos].type, tokens[pos].str);
        *success = false;
        return 0;
    }
    
    return result;
}