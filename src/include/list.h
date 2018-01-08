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

#ifndef _LIST_H_
#define _LIST_H_

#include <types.h>

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_rev(pos, head) \
        for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_entry(p, t, m)             container_of(p, t, m)

void init_list_head(struct list_head *head);
char list_empty(struct list_head *head);
void list_add(struct list_head *n, struct list_head *head);
void list_add_tail(struct list_head *n, struct list_head *head);
void list_del(struct list_head *e);

typedef int (*fn_list_cmp)(struct list_head *a, struct list_head *b);
void list_add_sorted(struct list_head *n, struct list_head *head,
                     fn_list_cmp cmp);
#endif
