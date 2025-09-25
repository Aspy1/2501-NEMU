#include "cpu/exec/helper.h"  // 包含helper函数所需的头文件

// 处理立即数形式的call指令（如 call 0x1234）
make_helper(call_si) {
    // 解码立即数操作数，获取指令长度（不包括操作码）
    int len = decode_si_l(eip + 1);
    
    // 计算返回地址：当前eip + 指令长度 + 操作码长度(1字节)
    swaddr_t ret_addr = cpu.eip + len + 1;
    
    // 将返回地址压入栈中（写入栈顶上方4字节位置）
    swaddr_write(cpu.esp - 4, 4, ret_addr);
    
    // 更新栈指针（栈向低地址增长）
    cpu.esp -= 4;
    
    // 跳转到目标地址：当前eip + 立即数偏移量
    cpu.eip += op_src->val;
    
    // 打印反汇编信息：显示目标地址
    print_asm("call %x", cpu.eip + 1 + len);
    
    // 返回指令总长度（操作码1字节 + 操作数长度）
    return len + 1;
}

// 处理寄存器/内存操作数形式的call指令（如 call *%eax）
make_helper(call_rm) {
    // 解码寄存器/内存操作数，获取指令长度（不包括操作码）
    int len = decode_rm_l(eip + 1);
    
    // 计算返回地址：当前eip + 指令长度 + 操作码长度(1字节)
    swaddr_t ret_addr = cpu.eip + len + 1;
    
    // 将返回地址压入栈中
    swaddr_write(cpu.esp - 4, 4, ret_addr);
    
    // 更新栈指针
    cpu.esp -= 4;
    
    // 跳转到目标地址：操作数值减去指令长度（调整eip位置）
    cpu.eip = op_src->val - (len + 1);
    
    // 打印反汇编信息：显示操作数
    print_asm("call *%s", op_src->str);
    
    // 返回指令总长度（操作码1字节 + 操作数长度）
    return len + 1;
}