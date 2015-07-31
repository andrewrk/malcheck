# malcheck

Test your library with malcheck to make sure it handles out of memory
conditions correctly.

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
$ ./malcheck ./test
malloc some memory!
malcheck initialized. Allocation index -1 will return NULL.
ptr1: 0x2123010
ptr2: 0x2123420
```

The only thing that changed is that we see a message on stderr. by default,
malcheck does not do any destructive behavior and your program should work
like normal. This just makes sure `malcheck` successfully was able to override
malloc/calloc/realloc/free.

So now try this:

```
$ ./malcheck --fail-index 0 ./test
malloc some memory!
malcheck initialized. Allocation index 0 will return NULL.
ptr1: (nil)
handle ptr1 error
ptr2: 0x120d010
```

And with fail index 1...

```
$ ./malcheck --fail-index 1 ./test
malloc some memory!
malcheck initialized. Allocation index 1 will return NULL.
ptr1: 0xfc2010
ptr2: (nil)
handle ptr2 error
```

## Building and Running

```
gcc -shared -o libmalcheck.so libmalcheck.c -fPIC -ldl
gcc -o malcheck malcheck.c
./malcheck path/to/exe [--fail-index N] [--libmalcheck libmalcheck.so] [-- args]
```
