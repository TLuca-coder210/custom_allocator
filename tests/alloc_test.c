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
    printf("Succes\n");
}

void test_realloc(void) {
    int *p1 = (int *)my_alloc(4 * sizeof(int));
    assert(p1 != NULL);
    p1[0] = 10;
    p1[3] = 40;
    int *p2 = (int *)my_realloc(p1, 8 * sizeof(int));
    assert(p2 != NULL);
    assert(p2[0] == 10);
    assert(p2[3] == 40);
    p2[7] = 80;
    assert(p2[7] == 80);
    int *p3 = (int *)my_realloc(p2, 2 * sizeof(int));
    assert(p3 != NULL);
    assert(p3[0] == 10);
    void *p4 = my_realloc(p3, 0);
    assert(p4 == NULL);
    void *p5 = my_realloc(NULL, 32);
    assert(p5 != NULL);
    assert(((uintptr_t)p5 % 16) == 0);
    my_free(p5);
    printf("Succes\n");
}

int main(void){
    test_basic_alloc();
    test_multiple_allocs();
    test_realloc();
}
