#include <stdio.h>
#include "heap.h"
int main() {
    int status = heap_setup();
    printf("%d %d\n",status,0);
    void *ptr = heap_malloc(795);
    heap_validate();
    printf("%p %p\n",ptr,NULL);
    printf("%d %d\n",pointer_valid,get_pointer_type(ptr));
    status = heap_validate();
    printf("%d %d\n",status,0);

    heap_clean();

    return 0;
}
