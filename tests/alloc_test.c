#include "my_alloc.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_alloc(void){
    void *ptr = my_alloc(64);
    assert(ptr != NULL);
    assert(((uintptr_t)ptr % 16) == 0);
    int *arr = (int*)ptr;
    arr[0] = 42;
    arr[1] = 100;
    assert(arr[0] == 42);
    assert(arr[1] == 100);
    my_free(ptr);
    printf("Succes\n");
}

void test_multiple_allocs(void){
    char *ptr1 = (char*)my_alloc(5);
    char *ptr2 = (char*)my_alloc(17);
    char *ptr3 = (char*)my_alloc(33);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);
    assert(((uintptr_t)ptr1 % 16) == 0);
    assert(((uintptr_t)ptr2 % 16) == 0);
    assert(((uintptr_t)ptr3 % 16) == 0);
    assert((uintptr_t)ptr2 > (uintptr_t)ptr1);
    assert((uintptr_t)ptr3 > (uintptr_t)ptr2);
    ptr1[0] = 'A';
    ptr1[4] = 'Z';
    ptr2[0] = 'X';
    ptr2[16] = 'Y';
    ptr3[0] = 'M';
    ptr3[32] = 'N';
    assert(ptr1[0] == 'A' && ptr1[4] == 'Z');
    assert(ptr2[0] == 'X' && ptr2[16] == 'Y');
    assert(ptr3[0] == 'M' && ptr3[32] == 'N');
    my_free(ptr1);
    my_free(ptr2);
    my_free(ptr3);
    printf("Succes");
}

int main(void){
    test_basic_alloc();
    test_multiple_allocs();
}
