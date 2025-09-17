#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP]; // 监视点池
//wp_pool是一个WP类型的数组，成员数为32
static WP *head, *free_;  // head 指向已使用的监视点链表头，free_ 指向空闲的监视点链表头
//二者是指向WP结构体的指针

WP* get_head_wp() {
    return head;
}
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

//监视点池管理功能
WP* new_wp(); //从free_读取并返回一个空闲的监视点。
void free_wp(WP* wp); //将wp释放到free_链表中

/* TODO: Implement the functionality of watchpoint */
// 翻译：实现监视点的功能

WP* new_wp(){
	//若没有空闲监视点
	if(free_ == NULL){ //表示指针为空 使用NULL
		printf("没有空闲监视点\n");
		assert(0);
	}
	else{
		// 在head中插入成员，删除free_的成员

        WP* new_wp = free_; // new_wp指向free_的第一个成员
        free_ = free_->next; // free_指向下一个成员

        // 初始化新添加的成员
        new_wp->expr[0] = '\0';  // 清空表达式
        new_wp->value = 0;       // 初始化值为0

        new_wp->next = head; // new_wp的next指向head
        head = new_wp;       // head指向new_wp

        return new_wp;      // 返回new_wp
	}
}

void free_wp(WP* wp){
	//在free_中插入成员，删除head的成员

	if(head == NULL){ //head为空
		printf("没有已使用监视点\n");
		assert(0);
	}
	else{
		WP* cur = head;
		WP* prev = NULL;

		//找到wp
		while(cur != NULL && cur != wp){
			prev = cur;
			cur = cur->next;
		}

		if(cur == NULL){ //没找到wp
			printf("监视点不存在\n");
			assert(0);
		}
		else{
			//删除cur
			if(prev == NULL){ //cur是head
				head = cur->next;
			}
			else{
				prev->next = cur->next;  //prev的next指向cur的下一个成员
			}

			//插入到free_
			cur->next = free_;
			free_ = cur;
		}
	}
}

//接下来实现监视点的功能

