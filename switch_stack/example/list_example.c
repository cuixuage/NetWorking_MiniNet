#include "../include/list.h"

#include <stdio.h>
#include <stdlib.h>

struct listnode {
	struct list_head list;
	int number;
};

void example()
{
	struct list_head list;
	init_list_head(&list);

	for (int i = 0; i < 10; i++) {
		struct listnode *node = malloc(sizeof(struct listnode));
		node->number = i;
		list_add_tail(&node->list, &list);         //注意 & 取地址符优先级最低   //这里的思想很好！！和pthread_mutex_lock思想类似
																				 //把结构体中加入链表元素  相当于list<struct>
																				 //mutex加锁只需要加锁某一结构体中pthread_mutex_t元素
	}

	fprintf(stdout, "list all numbers:\n");
	struct listnode *entry;
	list_for_each_entry(entry, &list, list) {
		fprintf(stdout, "%d\n", entry->number);
	}

	fprintf(stdout, "list only odd numbers and remove others:\n");
	struct listnode *q;
	list_for_each_entry_safe(entry, q, &list, list) {
		if (entry->number % 2 == 0) {
			list_delete_entry(&entry->list);
			free(entry);
		}
		else {
			fprintf(stdout, "%d\n", entry->number);
		}
	}
}

void main()
{
	example();
}
