#include "cpu/exec/template-start.h"

// 定义当前指令名称为push，这将影响生成的函数名和调试输出
#define instr push

// 实现push指令的核心执行逻辑
static void do_execute() {
    // 将源操作数的值写入栈顶（esp-4位置），使用4字节写入
    // 参数说明：
    // - cpu.esp - 4: 栈顶上方4字节的位置（栈向低地址增长）
    // - 4: 写入的数据长度（4字节，32位）
    // - op_src->val: 源操作数的值
    swaddr_write(cpu.esp - 4, 4, op_src->val);
    
    // 更新栈指针，模拟栈的push操作（栈指针减4，因为栈向低地址增长）
    cpu.esp -= 4;
    
    // 打印反汇编信息，template1表示单操作数指令
    print_asm_template1();
}

// 条件编译：只有在操作数大小为2或4字节时才生成以下helper函数
#if DATA_BYTE == 2 || DATA_BYTE == 4
    // 生成寄存器操作数形式的push指令helper（如：push %eax）
    make_instr_helper(r)
    
    // 生成寄存器/内存操作数形式的push指令helper（如：push (%ebx)）
    make_instr_helper(rm)
#endif

// 条件编译：只有在操作数大小为1字节时才生成以下helper函数
#if DATA_BYTE == 1
    // 生成符号扩展立即数形式的push指令helper
    make_instr_helper(si)
#endif
#include "cpu/exec/template-end.h"