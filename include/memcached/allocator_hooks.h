/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * Use this file as an abstraction to the underlying hooks api
 */

#ifndef ALLOCATOR_HOOKS_H
#define ALLOCATOR_HOOKS_H

#include <stdint.h>
#include <stdlib.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct allocator_ext_stat {
    char key[48];
    size_t value;
} allocator_ext_stat;

typedef struct allocator_stats {
    /* Bytes of memory allocated by the application. Doesn't include allocator
       overhead or fragmentation. */
    size_t allocated_size;

    /* Bytes of memory reserved by the allocator */
    size_t heap_size;

    /* free, mapped bytes (contributing to VSZ and RSS) */
    size_t free_mapped_size;

    /* free, unmapped bytes (contributing to VSZ) */
    size_t free_unmapped_size;

    /* Memory overhead of the allocator:
     *   heap_size - allocated_size - free_mapped_size - free_unmapped_size.
     */
    size_t fragmentation_size;

    /* Array of additional allocator-specific statistics, of size
       `ext_stats_size` */
    allocator_ext_stat *ext_stats;
    size_t ext_stats_size;
} allocator_stats;

/**
 * Engine allocator hooks for memory tracking.
 */
typedef struct engine_allocator_hooks_v1 {

    /**
     * Add a hook into the memory allocator that will be called each
     * time memory is allocated from the heap. Returns true if the
     * hook was successfully registered with the allocator. Returns
     * false if the hook was not registered properly or if a hooks
     * API doesn't exist for the allocator in use.
     */
    bool (*add_new_hook)(void (*)(const void* ptr, size_t size));

    /**
     * Remove a hook from the memory allocator that will be called each
     * time memory is allocated from the heap. Returns true if the hook
     * was registered and removed and false if the specified hook is not
     * registered or if a hooks API doesn't exist for the allocator.
     */
    bool (*remove_new_hook)(void (*)(const void* ptr, size_t size));

    /**
     * Add a hook into the memory allocator that will be called each
     * time memory is freed from the heap. Returns true if the hook
     * was successfully registered with the allocator. Returns false
     * if the hook was not registered properly or if a hooks API
     * doesn't exist for the allocator in use.
     */
    bool (*add_delete_hook)(void (*)(const void* ptr));

    /**
     * Remove a hook from the memory allocator that will be called each
     * time memory is freed from the heap. Returns true if the hook was
     * registered and removed and false if the specified hook is not
     * registered or if a hooks API doesn't exist for the allocator.
     */
    bool (*remove_delete_hook)(void (*)(const void* ptr));

    /**
     * Returns the number of extra stats for the current allocator.
     */
    int (*get_extra_stats_size)(void);

    /**
     * Obtains relevant statistics from the allocator. Every allocator
     * is required to return total allocated bytes, total heap bytes,
     * total free bytes, and toal fragmented bytes. An allocator will
     * also provide a varying number of allocator specific stats
     */
    void (*get_allocator_stats)(allocator_stats*);

    /**
     * Returns the total bytes allocated by the allocator. This value
     * may be computed differently based on the allocator in use.
     */
    size_t (*get_allocation_size)(const void*);

    /**
     * Fills a buffer with special detailed allocator stats.
     */
    void (*get_detailed_stats)(char*, int);

    /**
     * Attempts to release free memory back to the OS.
     */
    void (*release_free_memory)(void);

    /**
     * Enables / disables per-thread caching by the allocator
     * __for the calling thread__. Returns if the thread cache was enabled
     * before the call.
     */
    bool (*enable_thread_cache)(bool enable);

    /**
     * Sets the current memory region/arena for allocation
     */
    bool (*set_allocator_arena)(size_t arenaid);

    /**
     * Returns the memory currently allocated in an arena
     */
    size_t (*get_arena_allocation_size)(size_t arenaid);

    /**
     * Get/Set specific allocator properties
     */
    bool (*get_allocator_property)(const char* name, void* value, size_t* size);
    bool (*set_allocator_property)(const char* name, void* value, size_t size);

} ALLOCATOR_HOOKS_API;

#ifdef __cplusplus
}
#endif

#endif /* ALLOCATOR_HOOKS_H */
