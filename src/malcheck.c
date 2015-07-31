#define _GNU_SOURCE

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>

extern char **environ;

static int usage(const char *arg0) {
    fprintf(stderr, "Usage: %s [--fail-index N] [--libmalcheck libmalcheck.so] exe [args]\n", arg0);
    return EXIT_FAILURE;
}

static inline bool str_has_prefix(const char *big_str, const char *prefix, int prefix_len) {
    return strncmp(big_str, prefix, prefix_len) == 0;
}

int main(int argc, char **argv) {
    const char *arg0 = argv[0];

    const char *libmalcheck_path = MALCHECK_LIBMALCHECKSO;
    char *child_exe_path = NULL;
    int fail_index = -1;

    char **child_argv = calloc(argc + 1, sizeof(char *));
    if (!child_argv) {
        fprintf(stderr, "failed to alloc mem for child argv\n");
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i += 1) {
        const char *arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-') {
            i += 1;
            if (i >= argc) {
                return usage(arg0);
            } else {
                if (strcmp(arg, "--fail-index") == 0) {
                    fail_index = atoi(argv[i]);
                } else if (strcmp(arg, "--libmalcheck") == 0) {
                    libmalcheck_path = argv[i];
                }
            }
        } else if (!child_exe_path) {
            child_exe_path = argv[i];
            i += 1;
            for (int j = 1; i < argc; i += 1, j += 1)
                child_argv[j] = argv[i];
            break;
        }
    }

    if (!child_exe_path)
        return usage(arg0);

    int environ_item_count = 2;
    int libmalcheck_path_len = strlen(libmalcheck_path);
    int needed_environ_size = strlen("LD_PRELOAD=:") + libmalcheck_path_len +
        strlen("MALCHECK_FAIL_INDEX=99999999999999");
    for (char **ptr = environ; *ptr; ptr += 1) {
        needed_environ_size += strlen(*ptr) + 1;
        environ_item_count += 1;
    }
    int ptr_table_size = (environ_item_count + 1) * sizeof(char *);
    needed_environ_size += ptr_table_size;

    char *new_environ_bytes = malloc(needed_environ_size);
    if (!new_environ_bytes) {
        fprintf(stderr, "failed to alloc mem for environment\n");
        return EXIT_FAILURE;
    }

    char **new_environ_table = (char **)new_environ_bytes;
    char *new_environ_data = new_environ_bytes + ptr_table_size;
    bool found_item = false;
    int ld_preload_prefix_size = strlen("LD_PRELOAD=");
    for (char **ptr = environ; *ptr; ptr += 1) {
        char *old_item = *ptr;
        int old_item_len = strlen(old_item);

        *new_environ_table = new_environ_data;
        new_environ_table += 1;

        if (str_has_prefix(old_item, "LD_PRELOAD=", ld_preload_prefix_size)) {
            fprintf(stderr, "item: %s\n", old_item);
            found_item = true;

            new_environ_data += sprintf(new_environ_data, "LD_PRELOAD=%s:%s",
                    libmalcheck_path,
                    old_item + ld_preload_prefix_size) + 1;
        } else {
            memcpy(new_environ_data, old_item, old_item_len + 1);
            new_environ_data += old_item_len + 1;
        }
    }
    if (!found_item) {
        *new_environ_table = new_environ_data;
        new_environ_table += 1;

        new_environ_data += sprintf(new_environ_data, "LD_PRELOAD=%s", libmalcheck_path) + 1;
    }
    {
        *new_environ_table = new_environ_data;
        new_environ_table += 1;

        new_environ_data += sprintf(new_environ_data, "MALCHECK_FAIL_INDEX=%d", fail_index) + 1;
    }
    *new_environ_table = NULL;

    pid_t child_id = fork();

    if (child_id == -1) {
        fprintf(stderr, "fork failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if (child_id == 0) {
        child_argv[0] = child_exe_path;
        execvpe(child_exe_path, child_argv, (char**)new_environ_bytes);
        fprintf(stderr, "unable to spawn %s: %s\n", child_exe_path, strerror(errno));
        return EXIT_FAILURE;
    }
    free(new_environ_bytes);

    int child_return_value;
    pid_t id = waitpid(child_id, &child_return_value, 0);
    if (id == -1) {
        fprintf(stderr, "waitpid failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (child_return_value)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
