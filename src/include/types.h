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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stddef.h>
#include <stdint.h>

#if __SCHAR_MAX__ != 0x7f || \
    __SHRT_MAX__ != 0x7fff || \
    __INT_MAX__ != 0x7fffffff || \
    __LONG_MAX__ != 0x7fffffffl || \
    __LONG_LONG_MAX__ != 0x7fffffffffffffffll

#error "unsupported compiler or architecture."
#endif

#define STRINGIZE(x) #x

#define container_of(p, t, m) \
	(t *)((char *)p - offsetof(t, m))

/* a must be a power of 2. */
#define ALIGN_DN(v, a)		((v) & ~((a) - 1))
#define ALIGN_UP(v, a)		(((v) + (a) - 1) & ~((a) - 1))
#define ALIGNED(v, a)		(((v) & ((a) - 1)) == 0)

/* Flag support. */
#define BF_MASK(f)		((1ull << (f ## _SZ)) - 1)
/* Shifted mask. */
#define BF_SMASK(f)		(BF_MASK(f) << f ## _POS)
#define BF_CLR(v, f)		((v) &= ~BF_SMASK(f))

/* Shifted values. */
#define BF_GET(v, f)		(((v) >> f ## _POS) & BF_MASK(f))
#define BF_SET(v, f, w) \
	do { \
		(v) |= ((w) & BF_MASK(f)) << f ## _POS; \
	} while (0)

static inline uintptr_t bits_mask(size_t sz)
{
	return (1ull << sz) - 1;
}

static inline uintptr_t bits_arrange(uintptr_t pos, size_t sz, uintptr_t val)
{
	return (val & bits_mask(sz)) << pos;
}

static inline uintptr_t bits_extract(uintptr_t val, uintptr_t pos, size_t sz)
{
	return (val >> pos) & bits_mask(sz);
}

#define bits_set(flag, val)	bits_arrange(flag ## _POS, flag ## _SZ, (val))
#define bits_get(val, flag)	bits_extract(val, flag ## _POS, flag ## _SZ)

static inline uintptr_t bits_extract_nodatashift(uintptr_t val, uintptr_t pos,
						 size_t sz)
{
	return val & (bits_mask(sz) << pos);
}

#define bits_pull(val, flag)						\
	bits_extract_nodatashift(val, flag ## _POS, flag ## _SZ)

#define bits_push(flag, val)						\
	bits_extract_nodatashift(val, flag ## _POS, flag ## _SZ)

static inline uintptr_t _bits_on(uintptr_t pos, size_t sz)
{
	return bits_mask(sz) << pos;
}

static inline uintptr_t _bits_off(uintptr_t pos, size_t sz)
{
	return ~_bits_on(pos, sz);
}

#define bits_on(flag)	_bits_on(flag ## _POS, flag ## _SZ)
#define bits_off(flag)	_bits_off(flag ## _POS, flag ## _SZ)

/* Non-shifted values. */
#define BF_PULL(v, f)		((v) & BF_SMASK(f))
#define BF_PUSH(v, f, w) \
	do { \
		(v) |= BF_PULL(w, f); \
	} while (0)

#define ARRAY_SIZE(a)		sizeof(a)/sizeof((a)[0])

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

/* Tags to assign to the functions. */
#define _ctx_hard
#define _ctx_soft
#define _ctx_sched
#define _ctx_proc
#define _ctx_init
#endif
