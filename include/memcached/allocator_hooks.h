
/**
 * Use this file as an abstraction to the underlying hooks api
 */

#ifndef ALLOCATOR_HOOKS_H
#define ALLOCATOR_HOOKS_H

#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __WIN32__
#undef interface
#endif

typedef struct allocator_stat {
    char* key;
    size_t value;
} allocator_stat;

/**
 * Engine allocator hooks for memory tracking.
 */
typedef struct engine_allocator_hooks_v1 {
    bool (*add_new_hook)(void (*)(const void* ptr, size_t size));
    bool (*remove_new_hook)(void (*)(const void* ptr, size_t size));
    bool (*add_delete_hook)(void (*)(const void* ptr));
    bool (*remove_delete_hook)(void (*)(const void* ptr));
    allocator_stat* (*get_allocator_stats)(int*);
    size_t (*get_allocation_size)(void*);
    size_t (*get_fragmented_size)(void);
    size_t (*get_allocated_size)(void);
} ALLOCATOR_HOOKS_API;

#ifdef __cplusplus
}
#endif

#endif /* ALLOCATOR_HOOKS_H */
