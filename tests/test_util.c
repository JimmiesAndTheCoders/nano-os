#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Include the utility header from our OS
#include "util.h"

#define RUN_TEST(test_func) do { \
    printf("Running " #test_func "... "); \
    test_func(); \
    printf("PASS\n"); \
} while(0)

void test_strlen() {
    assert(strlen("hello") == 5);
    assert(strlen("") == 0);
    assert(strlen("A") == 1);
}

void test_strcmp() {
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strcmp("abd", "abc") > 0);
    assert(strcmp("", "") == 0);
    assert(strcmp("a", "") > 0);
}

void test_append() {
    char buf[16] = "nano";
    append(buf, '-');
    append(buf, 'o');
    append(buf, 's');
    assert(strcmp(buf, "nano-os") == 0);
}

void test_backspace() {
    char buf[16] = "shell";
    backspace(buf);
    assert(strcmp(buf, "shel") == 0);
    backspace(buf);
    assert(strcmp(buf, "she") == 0);
    backspace(buf);
    backspace(buf);
    backspace(buf);
    assert(strcmp(buf, "") == 0);
    // Backspacing an empty string should not crash
    backspace(buf);
    assert(strcmp(buf, "") == 0);
}

void test_memory_copy() {
    char src[] = "kernel data";
    char dest[20] = {0};
    memory_copy(src, dest, 11);
    assert(strcmp(dest, "kernel data") == 0);
}

void test_itoa() {
    char buf[16];
    itoa(123, buf);
    assert(strcmp(buf, "123") == 0);
    itoa(-456, buf);
    assert(strcmp(buf, "-456") == 0);
    itoa(0, buf);
    assert(strcmp(buf, "0") == 0);
}

int main() {
    printf("=======================================\n");
    printf(" Nano OS - Utility Function Test Suite \n");
    printf("=======================================\n");

    RUN_TEST(test_strlen);
    RUN_TEST(test_strcmp);
    RUN_TEST(test_append);
    RUN_TEST(test_backspace);
    RUN_TEST(test_memory_copy);
    RUN_TEST(test_itoa);

    printf("=======================================\n");
    printf(" All utility tests passed successfully!\n");
    printf("=======================================\n");
    return 0;
}