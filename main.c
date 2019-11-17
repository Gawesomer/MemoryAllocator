#include <stdio.h>
#include "alloc.h"

int main(int argc, char *argv[]) {
    void *ptr0, *ptr1, *ptr2;
    M_Init(4096);
    ptr0 = M_Alloc(1024);
    printf("M_Alloc = %p\n", ptr0);
    ptr1 = M_Alloc(1500);
    printf("M_Alloc = %p\n", ptr1);
    ptr2 = M_Alloc(1456);
    printf("M_Alloc = %p\n", ptr2);
    printf("M_Free(%p) = %d\n", (void*)ptr2, M_Free(ptr2));
    printf("M_Free(%p) = %d\n", (void*)ptr2, M_Free(ptr0));
    printf("M_Free(%p) = %d\n", (void*)ptr2, M_Free(ptr1));

    return 0;
}
