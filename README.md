# malcheck

Test your code with malcheck to make sure it handles out of memory
conditions correctly.

Supported platforms: Linux.

## Synopsis

Run a program and cause malloc/calloc/realloc to return NULL after N number of
allocations.

Look at this example C code:

```c
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
```

Normally, it produces output like this:

```
malloc some memory!
ptr1: 0x22ad010
ptr2: 0x22ad420
```

Systems have so much memory and/or have
[overcommit](https://en.wikipedia.org/wiki/Memory_overcommitment) enabled which
prevents the error handling code from ever being run. But we still want to test
the error handling code.

So let's run with `malcheck`:

```
$ malcheck ./test
malloc some memory!
==malcheck== Loading malloc, free, calloc, and realloc.
==malcheck== Initialized. Allocation index -1 will return NULL.
ptr1: 0x9e7010
ptr2: 0x9e7420
==malcheck== shutdown. allocs: 2, frees: 2
```

Now we can see that malcheck is running, but it didn't change the behavior of
the test program. By default, malcheck does not do any destructive behavior
and your program should work like normal. This just makes sure `malcheck`
successfully was able to override malloc/calloc/realloc/free.

So now try this:

```
$ malcheck --fail-index 0 ./test
malloc some memory!
==malcheck== Loading malloc, free, calloc, and realloc.
==malcheck== Initialized. Allocation index 0 will return NULL.
==malcheck== malloc returning NULL. Stack trace:
==malcheck==   at malloc (libmalcheck.c:216)
==malcheck==   at main (simple.c:7)
ptr1: (nil)
handle ptr1 error
ptr2: 0x8d83b0
==malcheck== shutdown. allocs: 139, frees: 48
```

It printed a stack trace for the first allocation and made it return NULL.
The allocation count and free count are skewed because getting the stack trace
does plenty of allocations and frees.

And with fail index 1...

```
$ malcheck --fail-index 1 ./test
malloc some memory!
==malcheck== Loading malloc, free, calloc, and realloc.
==malcheck== Initialized. Allocation index 1 will return NULL.
ptr1: 0x1ea2010
==malcheck== malloc returning NULL. Stack trace:
==malcheck==   at malloc (libmalcheck.c:216)
==malcheck==   at main (simple.c:13)
ptr2: (nil)
handle ptr2 error
==malcheck== shutdown. allocs: 139, frees: 48
```

## Installing From Source

First make sure elfutils and libunwind are installed, then it's the standard
cmake installation.

```
mkdir build
cd build
cmake ..
make
sudo make install
```
