
#include "alloc_hooks.h"

static bool triedLoadFunctions = false;
static bool haveHooksFunctions = true;

static Alias addNewHook;
static Alias addDelHook;
static Alias removeNewHook;
static Alias removeDelHook;

static Alias getStatsProp;
static Alias getAllocSize;

void loadAllocatorFunctions() {
    if (triedLoadFunctions) {
        return;
    }
    triedLoadFunctions = true;

    void* handle = dlopen(NULL, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    addNewHook.ptr = dlsym(handle, "MallocHook_AddNewHook");
    addDelHook.ptr = dlsym(handle, "MallocHook_AddDeleteHook");
    removeNewHook.ptr = dlsym(handle, "MallocHook_RemoveNewHook");
    removeDelHook.ptr = dlsym(handle, "MallocHook_RemoveDeleteHook");

    getStatsProp.ptr = dlsym(handle, "MallocExtension_GetNumericProperty");

    getAllocSize.ptr = dlsym(handle, "MallocExtension_GetAllocatedSize");

    if (!addNewHook.ptr && !addDelHook.ptr && !removeNewHook.ptr && !removeDelHook.ptr) {
        haveHooksFunctions = false;
        get_stderr_logger()->log(EXTENSION_LOG_WARNING, NULL,
                                 "Couldn't find allocator hooks for memory tracking");
    }
}

bool mc_add_new_hook(void (*hook)(const void* ptr, size_t size)) {
    loadAllocatorFunctions();
    if (haveHooksFunctions) {
        return (addNewHook.func)(hook) ? true : false;
    }
    return false;
}

bool mc_remove_new_hook(void (*hook)(const void* ptr, size_t size)) {
    loadAllocatorFunctions();
    if (haveHooksFunctions) {
        return (removeNewHook.func)(hook) ? true : false;
    }
    return false;
}

bool mc_add_delete_hook(void (*hook)(const void* ptr)) {
    loadAllocatorFunctions();
    if (haveHooksFunctions) {
        return (addDelHook.func)(hook) ? true : false;
    }
    return false;
}

bool mc_remove_delete_hook(void (*hook)(const void* ptr)) {
    loadAllocatorFunctions();
    if (haveHooksFunctions) {
        return (removeDelHook.func)(hook) ? true : false;
    }
    return false;
}

allocator_stat* mc_get_allocator_stats(int* size) {
    if (getStatsProp.ptr) {
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
            (getStatsProp.func)(tcmalloc_stats_names[i], &((*(stats + i)).value));
        }

        return stats;
    }
    *size = 0;
    return NULL;
}

size_t mc_get_allocation_size(void* ptr) {
    if (getAllocSize.ptr) {
        return (size_t)(getAllocSize.func)(ptr);
    }
    return 0;
}
