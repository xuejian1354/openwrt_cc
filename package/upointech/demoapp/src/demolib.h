
#ifndef __DEMO_LIST_H
#define __DEMO_LIST_H

#ifndef NULL
#  ifdef __cplusplus
#  define NULL        (0L)
#  else /* !__cplusplus */
#  define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif

struct demo_list {
	void *elem;
	struct demo_list *next;
	struct demo_list *prev;
};

#define demo_init_list(l) { .next = l, .prev = l }

#define demo_list_for_each(__iterator, __list)				\
	for (__iterator = (__list)->next;				\
	     __iterator != __list;					\
	     __iterator = __iterator->next)

#define demo_list_for_each_safe(__iterator, __list, __next)		\
	for (__iterator = (__list)->next, __next = __iterator->next;	\
	     __iterator != __list;					\
	     __iterator = __next, __next = __next->next)

void demo_list_init(struct demo_list *list);
void demo_list_add(struct demo_list *head, struct demo_list *list);
void demo_list_del(struct demo_list *list);

#endif