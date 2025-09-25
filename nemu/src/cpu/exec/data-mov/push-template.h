#include "cpu/exec/template-start.h"

#define instr push

static void do_execute() {
    // 实现push指令的核心逻辑
    // 将操作数压入栈中
}

// 根据push指令的操作数类型选择合适的helper
make_instr_helper(r)  // 寄存器操作数
// 或者 make_instr_helper(rm)  // 寄存器/内存操作数

#include "cpu/exec/template-end.h"