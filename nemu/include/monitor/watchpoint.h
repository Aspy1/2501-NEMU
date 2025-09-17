#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;					 //监视点编号
	struct watchpoint *next; //此处next指向下一个监视点

	/* TODO: Add more members if necessary */
	//翻译：如果有必要，添加更多成员
	char expr[64];              // 存储监视点表达式
    uint32_t value;             // 存储表达式的当前值
	
} WP;

WP* get_head_wp();

#endif
