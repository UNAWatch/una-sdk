#include <new>
#include <cstdio>

void *operator new(std::size_t sz)
{
    void *p = std::malloc(sz);
    //printf("new      %p %d bytes\n", p, sz);
    return p;
}

void *operator new[](std::size_t sz) {
    void *p = std::malloc(sz);
    //printf("new[]    %p %d bytes\n", p, sz);
    return p;
}

void operator delete(void *ptr)
{
    //printf("delete   %p\n", ptr);
    free(ptr);
}

void operator delete[](void *ptr) {
    //printf("delete[] %p\n", ptr);
    free(ptr);
}
