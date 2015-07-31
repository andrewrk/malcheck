#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

static void *(*orig_malloc)(size_t size) = NULL;
static void (*orig_free)(void *ptr) = NULL;
static void *(*orig_calloc)(size_t nmemb, size_t size) = NULL;
static void *(*orig_realloc)(void *ptr, size_t size) = NULL;

atomic_bool initialized = ATOMIC_VAR_INIT(false);
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *prop_prefix = "MALCHECK_FAIL_INDEX=";
static const int prop_prefix_len = 20;

atomic_long next_alloc_index = ATOMIC_VAR_INIT(0);
long fail_index = -1;


__attribute__ ((cold))
__attribute__ ((noreturn))
__attribute__ ((format (printf, 1, 2)))
static void panic(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
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

static void init(void) {
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
    fprintf(stderr, "malcheck initialized. Allocation index %d will return NULL.\n", fail_index);
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

void *malloc(size_t size) {
    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    return (index == fail_index) ? NULL : orig_malloc(size);
}

void free(void *ptr) {
    thread_safe_init();

    return orig_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    return (index == fail_index) ? NULL : orig_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    thread_safe_init();

    long index = atomic_fetch_add(&next_alloc_index, 1);
    return (index == fail_index) ? NULL : orig_realloc(ptr, size);
}
