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

#include <types.h>

void *memcpy(void *dst, const void *src, size_t l)
{
	int i;
	const unsigned char *s = src;
	unsigned char *d;

	d = dst;
	for (i = l >> 3; i > 0; --i) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (l & 4) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (l & 2) {
		*d++ = *s++;
		*d++ = *s++;
	}

	if (l & 1)
		*d++ = *s++;

	return dst;
}

void *memset(void *d, int v, size_t l)
{
	unsigned char *p;

	p = (unsigned char *)d;
	while (l) {
		*p = v;
		++p;
		--l;
	}
	return d;
}
