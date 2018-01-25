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
#include <irq.h>
#include <uart.h>

char *asrt_msg;
char *asrt_file;
int asrt_line;
void assert_fail(const char *a, const char *f, int l)
{
	asrt_msg = (char *)a;
	asrt_file = (char *)f;
	asrt_line = l;

	irq_disable();

	uart_send('!');
	uart_send_str(a);
	uart_send(' ');
	uart_send_str(f);
	uart_send(':');
	uart_send_num(l);

	while (1)
		wfi();
}
