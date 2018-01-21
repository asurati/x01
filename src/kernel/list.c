/*
 * Copyright (c) 2018 Amol Surati
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <list.h>

void init_list_head(struct list_head *head)
{
	head->next = head->prev = head;
}

char list_empty(struct list_head *head)
{
	return head->next == head;
}

static void list_add_between(struct list_head *n,
			     struct list_head *prev,
			     struct list_head *next)
{
	n->next = next;
	n->prev = prev;
	next->prev = n;
	prev->next = n;
}

void list_add(struct list_head *n, struct list_head *head)
{
	list_add_between(n, head, head->next);
}

void list_add_tail(struct list_head *n, struct list_head *head)
{
	list_add_between(n, head->prev, head);
}

void list_del(struct list_head *e)
{
	struct list_head *p, *n;
	n = e->next;
	p = e->prev;
	p->next = n;
	n->prev = p;
}

struct list_head *list_del_head(struct list_head *head)
{
	struct list_head *e;
	assert(!list_empty(head));
	e = head->next;
	list_del(e);
	return e;
}

void list_add_sorted(struct list_head *n, struct list_head *head,
		     fn_list_cmp cmp)
{
	int ret;
	struct list_head *e, *pos;
	pos = NULL;
	list_for_each(e, head) {
		ret = cmp(n, e);
		if (ret < 0)
			pos = e;
		else if (ret == 0)
			return;
	}

	if (pos == NULL)
		list_add_tail(n, head);
	else
		list_add_between(n, pos->prev, pos);
}

