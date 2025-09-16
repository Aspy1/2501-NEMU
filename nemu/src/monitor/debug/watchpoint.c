#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP]; // 监视点池
static WP *head, *free_;  // head 指向已使用的监视点链表头，free_ 指向空闲的监视点链表头

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;                //初始化每个监视点的编号 此处为0-31
		wp_pool[i].next = &wp_pool[i + 1];//成员next指向下一个监视点
	}
	wp_pool[NR_WP - 1].next = NULL;       //最后一个监视点的next指向NULL

	head = NULL;	//初始化，最初没有监视点被使用
	free_ = wp_pool;//最初整个监视点池都是空闲的
}//初始化

/* TODO: Implement the functionality of watchpoint */
// 翻译：实现监视点的功能

//监视点池管理功能
WP* new_wp(); //从free_读取并返回一个空闲的监视点