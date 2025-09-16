#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next; //此处next指向下一个监视点

	/* TODO: Add more members if necessary */
	//翻译：如果有必要，添加更多成员


} WP;

#endif
