
#include "demolib.h"


void demo_list_init(struct demo_list *list)
{
	list->elem = NULL;
	list->next = list->prev = list;
}


static void __demo_list_add(struct demo_list *new,
				  struct demo_list *prev,
				  struct demo_list *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void demo_list_add(struct demo_list *head, struct demo_list *list)
{
	__demo_list_add(list, head, head->next);
}

void demo_list_del(struct demo_list *list)
{
	struct demo_list *next, *prev;

	next = list->next;
	prev = list->prev;
	next->prev = prev;
	prev->next = next;
}
