/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "config.h"

#include <atomic>
#include <cstring>
#include <platform/cb_malloc.h>
#include <string>

#include "daemon/alloc_hooks.h"

#include <gtest/gtest.h>

// Test pointer in global scope to prevent compiler optimizing malloc/free away
// via DCE.
char* p;

template <typename T>
T get_allocator_property(const char* name) {
    T value = 0;
    size_t size=sizeof(T);
    AllocHooks::get_allocator_property(name, &value, &size);
    return value;
}

template <typename T>
bool set_allocator_property(const char* name, T value) {
    return AllocHooks::set_allocator_property(name, &value, sizeof(value));
}


class MemoryTrackerTest : public ::testing::Test {
public:
    // callback function for when memory is allocated.
    static void NewHook(const void* ptr, size_t) {
        if (ptr != NULL) {
            void* p = const_cast<void*>(ptr);
            //hook based
            hook_based_alloc_size += AllocHooks::get_allocation_size(p);
        }
    }

    // callback function for when memory is deleted.
    static void DeleteHook(const void* ptr) {
        if (ptr != NULL) {
            void* p = const_cast<void*>(ptr);
            hook_based_alloc_size -= AllocHooks::get_allocation_size(p);
        }
    }

    static size_t getArenaSize() {
        arena_based_alloc_size = AllocHooks::get_arena_allocation_size(5);
        return arena_based_alloc_size;
    }

    // function executed by our accounting test thread.
    static void AccountingTestThread(void* arg);

    // Helper function to lookup a symbol in a plugin and return a correctly-typed
    // function pointer.
    template <typename T>
    static T get_plugin_symbol(cb_dlhandle_t handle, const char* symbol_name) {
        char* errmsg;
        T symbol = reinterpret_cast<T>(cb_dlsym(handle, symbol_name, &errmsg));
        cb_assert(symbol != nullptr);
        return symbol;
    }

    static std::atomic_size_t arena_based_alloc_size;
    static std::atomic_size_t hook_based_alloc_size;

    static void  dumpArenaStats(size_t i = 0) {
#define OUT_FMT(n)  std::left << std::setw(n) << std::setprecision(n)
        getArenaSize();
        std::cout
            << OUT_FMT(5) << std::right << i << ":"
            << " arena:"  << get_allocator_property<unsigned>("thread.arena")
            << " small:" << OUT_FMT(15) << get_allocator_property<size_t>("stats.arenas.5.small.allocated")
            << " large:" << OUT_FMT(15) << get_allocator_property<size_t>("stats.arenas.5.large.allocated")
            << " huge:"  << OUT_FMT(15) << get_allocator_property<size_t>("stats.arenas.5.huge.allocated")
            << " arena_alloc:"  << OUT_FMT(15) << arena_based_alloc_size
            << " hook_alloc:" << OUT_FMT(15) << hook_based_alloc_size
            << " mapped:" << get_allocator_property<size_t>("stats.arenas.5.mapped")
            << " act.pgs:"<< get_allocator_property<size_t>("stats.arenas.5.pactive")
            << std::endl;
    }

};

std::atomic_size_t MemoryTrackerTest::arena_based_alloc_size;
std::atomic_size_t MemoryTrackerTest::hook_based_alloc_size;

void MemoryTrackerTest::AccountingTestThread(void* arg) {
    //AllocHooks::set_allocator_arena(5);
    set_allocator_property<unsigned>("thread.arena", 5);
    arena_based_alloc_size = 0;
    hook_based_alloc_size = 0;
    std::atomic_size_t last_alloc_size;

    // Test new & delete //////////////////////////////////////////////////
    p = new char();
    getArenaSize();
    EXPECT_GT(arena_based_alloc_size, 0);
    delete p;
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test new[] & delete[] //////////////////////////////////////////////
    p = new char[100];
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 100);

    delete []p;
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test random create and delete /////////////////////////////////////////
    for (size_t i = 0 ; i < 10 ; i++) {
        p = new char[(rand()%1000)*2048];
        delete p;
        EXPECT_EQ(0, arena_based_alloc_size);
    }

    // Test cb_malloc() / cb_free() /////////////////////////////////////////////
    p = static_cast<char*>(cb_malloc(sizeof(char) * 10));
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 10);
    cb_free(p);
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test cb_realloc() /////////////////////////////////////////////////////
    p = static_cast<char*>(cb_malloc(1));
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 1);

    // Allocator may round up allocation sizes; so it's hard to
    // accurately predict how much arena_based_alloc_size will increase. Hence
    // we just increase by a "large" amount and check at least half that
    // increment.
    last_alloc_size = getArenaSize();
    p = static_cast<char*>(cb_realloc(p, sizeof(char) * 100));
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, (last_alloc_size + 50));


    last_alloc_size = getArenaSize();
    p = static_cast<char*>(cb_realloc(p, 1));

    getArenaSize();
    EXPECT_LT(arena_based_alloc_size, last_alloc_size);

    last_alloc_size = getArenaSize();
    char* q = static_cast<char*>(cb_realloc(NULL, 10));
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, last_alloc_size + 10);

    cb_free(p);
    cb_free(q);
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test cb_calloc() //////////////////////////////////////////////////////
    p = static_cast<char*>(cb_calloc(sizeof(char), 20));

    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 20);
    cb_free(p);

    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test indirect use of malloc() via cb_strdup() /////////////////////////
    p = cb_strdup("random string");

    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, sizeof("random string"));
    cb_free(p);

    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    // Test memory allocations performed from another shared library loaded
    // at runtime.
    char* errmsg = nullptr;
    cb_dlhandle_t plugin = cb_dlopen("memcached_memory_tracking_plugin",
                                     &errmsg);
    EXPECT_NE(plugin, nullptr);

    // dlopen()ing a plugin can allocate memory. Reset arena_based_alloc_size.
    last_alloc_size = getArenaSize();

    typedef void* (*plugin_malloc_t)(size_t);
    auto plugin_malloc =
        get_plugin_symbol<plugin_malloc_t>(plugin, "plugin_malloc");
    p = static_cast<char*>(plugin_malloc(100));
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, last_alloc_size+100);

    typedef void (*plugin_free_t)(void*);
    auto plugin_free =
        get_plugin_symbol<plugin_free_t>(plugin, "plugin_free");
    plugin_free(p);
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    typedef char* (*plugin_new_char_t)(size_t);
    auto plugin_new_char =
        get_plugin_symbol<plugin_new_char_t>(plugin, "plugin_new_char_array");
    p = plugin_new_char(200);
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 200);

    typedef void (*plugin_delete_array_t)(char*);
    auto plugin_delete_char =
        get_plugin_symbol<plugin_delete_array_t>(plugin, "plugin_delete_array");
    plugin_delete_char(p);
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    typedef std::string* (*plugin_new_string_t)(const char*);
    auto plugin_new_string =
        get_plugin_symbol<plugin_new_string_t>(plugin, "plugin_new_string");
    auto* string = plugin_new_string("duplicate_string");
    getArenaSize();
    EXPECT_GE(arena_based_alloc_size, 16);

    typedef void(*plugin_delete_string_t)(std::string* ptr);
    auto plugin_delete_string =
        get_plugin_symbol<plugin_delete_string_t>(plugin, "plugin_delete_string");
    plugin_delete_string(string);
    getArenaSize();
    EXPECT_EQ(0, arena_based_alloc_size);

    cb_dlclose(plugin);
}

// Test that the various memory allocation / deletion functions are correctly
// accounted for, when run in a parallel thread.
TEST_F(MemoryTrackerTest, Accounting) {
    // AllocHooks::set_allocator_arena(0);
    set_allocator_property<unsigned>("thread.arena", 0);
    mc_add_new_hook(NewHook);
    mc_add_delete_hook(DeleteHook);

    std::cout << "num.arenas : " << get_allocator_property<unsigned>("opt.narenas") << std::endl;
    for (auto i = 0; i < 10; i++ ) {
        auto propVal = get_allocator_property<unsigned>("arenas.extend");
        std::cout << "num.arenas : " << get_allocator_property<unsigned>("arenas.narenas") << " "
                  << "new.arena.id:" << propVal << std::endl;
        // AllocHooks::set_allocator_arena(propVal);
    }
    std::cout << "main thread : " << get_allocator_property<unsigned>("thread.arena") << std::endl;
    return;
    cb_thread_t tid;
    ASSERT_EQ(0, cb_create_thread(&tid, AccountingTestThread, 0, 0));
    ASSERT_EQ(0, cb_join_thread(tid));

    mc_remove_new_hook(NewHook);
    mc_remove_delete_hook(DeleteHook);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    init_alloc_hooks();

    return RUN_ALL_TESTS();
}
