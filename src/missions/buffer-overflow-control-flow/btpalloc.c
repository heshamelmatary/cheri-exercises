/*
 * SPDX-License-Identifier: BSD-2-Clause-DARPA-SSITH-ECATS-HR0011-18-C-0016
 * Copyright (c) 2020 Jessica Clarke
 */
#include "btpalloc.h"

#include <stddef.h>
#include <printf.h>
#include <sel4/assert.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif

void *btpmem;
size_t btpmem_size;

void *
btpmalloc(size_t size)
{
	void *alloc;
	size_t allocsize;

	/* Microkit should have mapped and patched btpmem memory region during bootstrapping */
	seL4_Assert(btpmem != NULL);
	seL4_Assert(btpmem_size != 0);

	printf("btpmem = 0x%lx\n", (size_t) btpmem);
	printf("btpmemsize = 0x%lx\n", (size_t) btpmem_size);

	alloc = btpmem;
	/* RISC-V ABIs require 16-byte alignment */
	allocsize = __builtin_align_up(size, 16);

#if defined(__CHERI_PURE_CAPABILITY__) && !defined(CHERI_NO_ALIGN_PAD)
	allocsize = cheri_representable_length(allocsize);
	alloc = __builtin_align_up(alloc,
	    ~cheri_representable_alignment_mask(allocsize) + 1);
	allocsize += (char *)alloc - (char *)btpmem;
#endif

	if (allocsize > btpmem_size)
		return (NULL);

	btpmem = (char *)btpmem + allocsize;
	btpmem_size -= allocsize;
#ifdef __CHERI_PURE_CAPABILITY__
	alloc = cheri_bounds_set(alloc, size);
#endif
	printf("Returning alloc = 0x%lx\n", (size_t) alloc);
	return (alloc);
}

void
btpfree(void *ptr)
{
	(void)ptr;
}
