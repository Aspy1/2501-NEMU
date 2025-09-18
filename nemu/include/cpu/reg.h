#ifndef __REG_H__
#define __REG_H__
//因为删除了CPU_state结构体中的重复寄存器定义，需要把这些寄存器的访问宏定义补充上
#define eax gpr[R_EAX]._32
#define ecx gpr[R_ECX]._32
#define edx gpr[R_EDX]._32
#define ebx gpr[R_EBX]._32
#define esp gpr[R_ESP]._32
#define ebp gpr[R_EBP]._32
#define esi gpr[R_ESI]._32
#define edi gpr[R_EDI]._32
#include "common.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };
//定义了三个枚举类型，分别表示32位、16位和8位寄存器的索引 enum允许程序员为一组相关的整数常量赋予有意义的名称
//例如 R_EAX 对应 0，R_ECX 对应 1，以此类推

//这反映了i386架构中寄存器的层次结构：32位寄存器（如EAX）、16位寄存器（如AX）和8位寄存器（如AL和AH）。
//试图实现的功能：提供寄存器索引的映射，方便代码中通过索引访问寄存器。

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */
/*题目翻译：重新组织 CPU_state 结构以匹配 i386 指令格式中的寄存器编码方案。
例如，如果我们访问 cpu.gpr[3]._16，我们将获得 'bx' 寄存器；
如果我们访问 cpu.gpr[1]._8[1]，我们将获得 'ch' 寄存器。
提示：使用 'union'。有关寄存器编码方案的更多详细信息，请参阅 i386 手册。*/

typedef struct {
     union {
        uint32_t _32;
        uint16_t _16;
        uint8_t _8[2];
     } gpr[8];
//上述结构体的意义是：定义了一个包含8个通用寄存器的结构体，每个寄存器可以访问32位、16位和8位的值。
//此处存在错误：它们没有使用union，值不会同步变化，违反i386原则 修改为使用union
     /* Do NOT change the order of the GPRs' definitions. */
     //翻译：不要更改通用寄存器定义的顺序
     // uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
     //定义了8个32位的寄存器变量，分别对应通用寄存器数组中的每个寄存器
     //这样的定义造成重复，不符合i386原则
     swaddr_t eip;
     //定义了一个 swaddr_t 类型的变量 eip，表示指令指针寄存器 swaddr_t 在 common.h 中被定义为 uint32_t 类型
     union {
        struct {
            uint32_t CF		:1;
            uint32_t pad0	:1;
            uint32_t PF		:1;
            uint32_t pad1	:1;
            uint32_t AF		:1;
            uint32_t pad2	:1;
            uint32_t ZF		:1;
            uint32_t SF		:1;
            uint32_t TF		:1;
            uint32_t IF		:1;
            uint32_t DF		:1;
            uint32_t OF		:1;
            uint32_t IOPL	:2;
            uint32_t NT		:1;
            uint32_t pad3	:1;
            uint16_t pad4;
        };
        uint32_t val;
    } eflags;
//定义了一个联合体 eflags，包含一个按位定义的结构体和一个32位整数 val，用于表示和操作 EFLAGS 寄存器的各个位标志
} CPU_state;
//定义了一个名为 CPU_state 的结构体，表示 CPU 的状态，包括通用寄存器、指令指针和标志寄存器

extern CPU_state cpu;
//声明了一个外部变量 cpu，类型为 CPU_state，表示当前 CPU 的状态

static inline int check_reg_index(int index) {
    assert(index >= 0 && index < 8);
    return index;
}
//定义了一个内联函数 check_reg_index，用于检查寄存器索引的有效性，确保索引在0到7之间
#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
//#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])
//reg_b的运算方式有误 例如，对于AH（索引4），计算后访问的是CH（gpr[1]._8[1]），而不是AH（gpr[0]._8[1]）
//后四个寄存器（索引4-7）没有高8位，但宏仍然允许访问
#define reg_b(index) (cpu.gpr[check_reg_index(index & 0x3)]._8[index >> 2])
//修正后的reg_b宏：低三位决定gpr索引，高位决定访问低8位还是高8位
extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];
//声明了三个外部字符串数组，分别表示32位、16位和8位寄存器的名称
#endif
//标记完成 