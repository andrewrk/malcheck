#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>

extern char **environ;

static void *(*orig_malloc)(size_t size) = NULL;
static void (*orig_free)(void *ptr) = NULL;
static void *(*orig_calloc)(size_t nmemb, size_t size) = NULL;
static void *(*orig_realloc)(void *ptr, size_t size) = NULL;

static atomic_bool initialized = ATOMIC_VAR_INIT(false);
static __thread bool bootstrapping = false;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *prop_prefix = "MALCHECK_FAIL_INDEX=";
static const int prop_prefix_len = 20;

static atomic_long next_alloc_index = ATOMIC_VAR_INIT(0);
static long fail_index = -1;

static long alloc_count = 0;
static long free_count = 0;

// dlsym itself allocates memory, so this is what we give it.
// 32KB should be enough.
#define BOOTSTRAP_MEM_SIZE (32 * 1024)
static char hunk_o_memory[BOOTSTRAP_MEM_SIZE];
static size_t next_hunk_index = 0;

__attribute__ ((format (printf, 1, 2)))
static void errlog(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "==malcheck== ");
    vfprintf(stderr, format, ap);
    va_end(ap);
}

__attribute__ ((cold))
__attribute__ ((noreturn))
__attribute__ ((format (printf, 1, 2)))
static void panic(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "==malcheck== ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    abort();
}

static inline bool str_has_prefix(const char *big_str, const char *prefix, int prefix_len) {
    if (prefix_len == -1)
        prefix_len = strlen(prefix);
    return strncmp(big_str, prefix, prefix_len) == 0;
}

static void malcheck_on_exit(void) {
    errlog("shutdown. allocs: %ld, frees: %ld\n", alloc_count, free_count);
}

static void init(void) {
    bootstrapping = true;

    errlog("Loading malloc, free, calloc, and realloc.\n");
    orig_malloc = dlsym(RTLD_NEXT, "malloc");
    if (!orig_malloc)
        panic("unable to get malloc pointer: %s", dlerror());

    orig_free = dlsym(RTLD_NEXT, "free");
    if (!orig_free)
        panic("unable to get free pointer: %s", dlerror());

    orig_calloc = dlsym(RTLD_NEXT, "calloc");
    if (!orig_free)
        panic("unable to get calloc pointer: %s", dlerror());

    orig_realloc = dlsym(RTLD_NEXT, "realloc");
    if (!orig_free)
        panic("unable to get realloc pointer: %s", dlerror());

    for (char **ptr = environ; *ptr; ptr += 1) {
        char *item = *ptr;
        if (str_has_prefix(item, prop_prefix, prop_prefix_len)) {
            fail_index = atoi(item + prop_prefix_len);
        }
    }
    if (atexit(malcheck_on_exit)) {
        panic("unable to register atexit handler: %s", strerror(errno));
    }
    bootstrapping = false;

    errlog("Initialized. Allocation index %ld will return NULL.\n", fail_index);
}

static void thread_safe_init(void) {
    if (atomic_load(&initialized))
        return;

    pthread_mutex_lock(&init_mutex);

    if (atomic_load(&initialized)) {
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    atomic_store(&initialized, true);

    init();

    pthread_mutex_unlock(&init_mutex);
}

static void print_trace(void) {
    void *array[256];

    size_t size = backtrace (array, 256);
    char **strings = backtrace_symbols (array, size);

    for (size_t i = 0; i < size; i++)
        errlog ("%s\n", strings[i]);

    free (strings);
}

static void *bootstrap_malloc(size_t size) {
    char *mem = &hunk_o_memory[next_hunk_index];
    next_hunk_index += size;
    if (next_hunk_index >= BOOTSTRAP_MEM_SIZE)
        panic("bootstrap memory exceeded");
    return mem;
}

static void *bootstrap_calloc(size_t nmemb, size_t size) {
    char *mem = bootstrap_malloc(nmemb * size);
    memset(mem, 0, size);
    return mem;
}

void *malloc(size_t size) {
    if (bootstrapping)
        return bootstrap_malloc(size);
    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    if (index == fail_index) {
        errlog("malloc returning NULL. Stack trace:\n");
        print_trace();
        return NULL;
    }

    void *ptr = orig_malloc(size);

    if (ptr)
        alloc_count += 1;

    return ptr;
}

void free(void *ptr) {
    if (bootstrapping)
        return;

    thread_safe_init();

    if (ptr)
        free_count += 1;

    return orig_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    if (bootstrapping)
        return bootstrap_calloc(nmemb, size);

    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    if (index == fail_index) {
        errlog("calloc returning NULL. Stack trace:\n");
        print_trace();
        return NULL;
    }

    void *ptr = orig_calloc(nmemb, size);
    if (ptr)
        alloc_count += 1;

    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (bootstrapping)
        panic("bootstrap realloc not supported");

    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    if (index == fail_index) {
        errlog("realloc returning NULL. Stack trace:\n");
        print_trace();
        return NULL;
    }

    return orig_realloc(ptr, size);
}
