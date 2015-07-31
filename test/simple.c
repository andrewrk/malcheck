#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    fprintf(stderr, "malloc some memory!\n");

    void *ptr1 = malloc(1024);
    fprintf(stderr, "ptr1: %p\n", ptr1);

    if (!ptr1)
        fprintf(stderr, "handle ptr1 error\n");

    void *ptr2 = malloc(28);
    fprintf(stderr, "ptr2: %p\n", ptr2);

    if (!ptr2)
        fprintf(stderr, "handle ptr2 error\n");


    free(ptr1);
    free(ptr2);

    return 0;
}
