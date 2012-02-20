
#ifndef MEM_HOOKS_H
#define MEM_HOOKS_H

#include "config.h"
#include <memcached/allocator_hooks.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(HAVE_LIBTCMALLOC) || defined (HAVE_LIBTCMALLOC_MINIMAL)
#include <google/malloc_extension_c.h>
#include <google/malloc_hook_c.h>
#endif

bool mc_add_new_hook(void (*)(const void* ptr, size_t size));
bool mc_remove_new_hook(void (*)(const void* ptr, size_t size));
bool mc_add_delete_hook(void (*)(const void* ptr));
bool mc_remove_delete_hook(void (*)(const void* ptr));
allocator_stat* mc_get_allocator_stats(int*);
size_t mc_get_allocation_size(void*);

#endif /* MEM_HOOKS_H */
