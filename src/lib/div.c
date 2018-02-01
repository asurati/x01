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

#include <div.h>

unsigned int __aeabi_uidiv(unsigned int dvd, unsigned int dvsr)
{
	unsigned int q;
	if (dvsr == 0)
		return 0;
	q = 0;
	while (dvd >= dvsr) {
		++q;
		dvd -= dvsr;
	}
	return q;
}
