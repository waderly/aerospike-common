/*
 *  Citrusleaf Foundation
 *  include/alloc.h - memory allocation framework
 *
 *  Copyright 2008 by Citrusleaf.  All rights reserved.
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 *  ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 */
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "cf.h"


/* cf_rc_count
 * Get the reservation count for a memory region */
cf_atomic_int_t
cf_rc_count(void *addr)
{
	cf_rc_counter *rc;
	cf_assert(addr, CF_RCALLOC, PROCESS, CRITICAL, "null address");

	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

	return(*rc);
}


/* cf_rc_reserve
 * Get a reservation on a memory region */
int
cf_rc_reserve(void *addr)
{
	cf_rc_counter *rc;
	cf_assert(addr, CF_RCALLOC, PROCESS, CRITICAL, "null address");

	/* Extract the address of the reference counter, then increment it */
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

//	return(cf_atomic_int_addunless(rc, 0, 1));
	return(cf_atomic_int_add(rc, 1));
}


/* _cf_rc_release
 * Release a reservation on a memory region */
cf_atomic_int_t
_cf_rc_release(void *addr, bool autofree)
{
	cf_rc_counter *rc;
	uint64_t c;
	cf_assert(addr, CF_RCALLOC, PROCESS, CRITICAL, "null address");

	/* Release the reservation; if this reduced the reference count to zero,
	 * then free the block if autofree is set, and return 1.  Otherwise,
	 * return 0 */
	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));
	if (0 == (c = cf_atomic_int_decr(rc)))
		if (autofree)
			free((void *)rc);

	return(c);
}


/* cf_rc_alloc
 * Allocate a reference-counted memory region.  This region will be filled
 * with bytes of value zero */
void *
cf_rc_alloc(size_t sz)
{
	byte *addr;
	size_t asz = sizeof(cf_rc_counter) + sz;

	addr = malloc(asz);
	if (NULL == addr)
		return(NULL);

	cf_atomic_int_set((cf_atomic_int *)addr, 1);
	byte *base = addr + sizeof(asz);

	return(base);
}


/* cf_rc_free
 * Deallocate a reference-counted memory region */
void
cf_rc_free(void *addr)
{
	cf_rc_counter *rc;
	cf_assert(addr, CF_RCALLOC, PROCESS, CRITICAL, "null address");

	rc = (cf_rc_counter *) (((byte *)addr) - sizeof(cf_rc_counter));

	cf_assert(cf_atomic_int_get(*(cf_atomic_int *)rc) == 0,
		CF_RCALLOC, PROCESS, CRITICAL, "attempt to free reserved object");

	free((void *)rc);

	return;
}
