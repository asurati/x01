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

#include <bdy.h>
#include <string.h>

size_t bdy_map_size(int n)
{
	int i, t;

	t = 0;
	for (i = 0; i < BDY_NLEVELS; ++i) {
		t += n;
		if (n & 1)
			++n;
		n >>= 1;
	}

	return ALIGN_UP(t, 8) >> 3;
}

void bdy_init(struct bdy *b, void *map, int n)
{
	int i, t;

	memset(b, 0, sizeof(*b));
	memset(map, 0, bdy_map_size(n));

	b->map = map;

	t = 0;
	for (i = 0; i < BDY_NLEVELS; ++i) {
		b->nbits[i] = n;
		b->nbits_until[i] = t;
		t += n;
		if (n & 1)
			++n;
		n >>= 1;
	}
}

#define BDY_LIMB(pos)		((pos) >> 5)
#define BDY_BIT(pos)		((pos) & 0x1f)
typedef uint32_t limb_t;

static char bdy_is_set(const struct bdy *b, int level, int pos)
{
	int l, p;
	limb_t *map;

	map = b->map;
	pos += b->nbits_until[level];
	l = BDY_LIMB(pos);
	p = BDY_BIT(pos);
	if (map[l] & (1 << p))
		return 1;
	else
		return 0;
}

static void bdy_clr(struct bdy *b, int level, int pos)
{
	int l, p;
	limb_t *map;

	map = b->map;
	pos += b->nbits_until[level];
	l = BDY_LIMB(pos);
	p = BDY_BIT(pos);
	map[l] &= ~(1 << p);
}

static void bdy_set(struct bdy *b, int level, int pos)
{
	int l, p;
	limb_t *map;

	map = b->map;
	pos += b->nbits_until[level];
	l = BDY_LIMB(pos);
	p = BDY_BIT(pos);
	map[l] |= 1 << p;
}

int bdy_alloc(struct bdy *b, int level, int *out)
{
	int i, j, k, pos;

	for (i = 0; i < b->nbits[level]; ++i)
		if (!bdy_is_set(b, level, i))
			break;

	if (i == b->nbits[level])
		return -1;

	pos = j = i;

	/* Self and Ancestor bits. */
	for (i = level; i < BDY_NLEVELS; ++i, j >>= 1)
		bdy_set(b, i, j);

	j = pos;
	/* Descendant bits. */
	for (i = level - 1; i >= 0; --i) {
		j <<= 1;
		for (k = 0; k < 1 << (level - i); ++k) {
			if (j + k >= b->nbits[i])
				break;
			bdy_set(b, i, j + k);
		}
	}

	if (out)
		*out = pos;
	return 0;
}

int bdy_free(struct bdy *b, int level, int pos)
{
	int i, j, k, s;

	j = pos;
	/* Descendant bits. */
	for (i = level - 1; i >= 0; --i) {
		j <<= 1;
		for (k = 0; k < 1 << (level - i); ++k) {
			if (j + k >= b->nbits[i])
				break;
			bdy_clr(b, i, j + k);
		}
	}

	j = pos;
	/* Self and Ancestor bits. */
	for (i = level; i < BDY_NLEVELS; ++i, j >>= 1) {
		bdy_clr(b, i, j);

		/* Check the sibling. */
		if (j & 1)
			s = j - 1;
		else
			s = j + 1;

		/* If the sibling exists and is set, exit. */
		if (s < b->nbits[i] && bdy_is_set(b, i, s))
			break;

		/* Else, the sibling either does not exist, or
		 * is clear. Continue clearing the parent.
		 */
	}
	return 0;
}

