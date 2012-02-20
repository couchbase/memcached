
#include "alloc_hooks.h"

bool mc_add_new_hook(void (*hook)(const void* ptr, size_t size)) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    return (MallocHook_AddNewHook(hook) == 1) ? true : false;
#else
    return false;
#endif
}

bool mc_remove_new_hook(void (*hook)(const void* ptr, size_t size)) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    return (MallocHook_RemoveNewHook(hook) == 1) ? true : false;
#else
    return false;
#endif
}

bool mc_add_delete_hook(void (*hook)(const void* ptr)) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    return (MallocHook_AddDeleteHook(hook) == 1) ? true : false;
#else
    return false;
#endif
}

bool mc_remove_delete_hook(void (*hook)(const void* ptr)) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    return (MallocHook_RemoveDeleteHook(hook) == 1) ? true : false;
#else
    return false;
#endif
}

allocator_stat* mc_get_allocator_stats(int* size) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    char* tcmalloc_stats_names[] = {"generic.current_allocated_bytes",
                                    "generic.heap_size",
                                    "tcmalloc.pageheap_free_bytes",
                                    "tcmalloc.pageheap_unmapped_bytes",
                                    "tcmalloc.max_total_thread_cache_bytes",
                                    "tcmalloc.current_total_thread_cache_bytes"};

    char* stats_names[] = {"tcmalloc_allocated_bytes",
                           "tcmalloc_heap_size",
                           "tcmalloc_free_bytes",
                           "tcmalloc_unmapped_bytes",
                           "tcmalloc_max_thread_cache_bytes",
                           "tcmalloc_current_thread_cache_bytes"};

    *size = sizeof(tcmalloc_stats_names) / sizeof(char*);
    allocator_stat* stats = (allocator_stat*)malloc(*size * sizeof(allocator_stat));

    int i;
    for (i = 0; i < *size; i++) {
        (*(stats + i)).key = strdup(stats_names[i]);
        MallocExtension_GetNumericProperty(tcmalloc_stats_names[i],
                                           &((*(stats + i)).value));
    }

    return stats;
#else
    *size = 0;
    return NULL;
#endif
}

size_t mc_get_allocation_size(void* ptr) {
#if defined(HAVE_LIBTCMALLOC) || defined(HAVE_LIBTCMALLOC_MINIMAL)
    return MallocExtension_GetAllocatedSize(ptr);
#else
    return 0;
#endif
}
