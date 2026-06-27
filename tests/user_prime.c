#include "stdio.h"
#include "unistd.h"

int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

int main(int argc, char** argv, char** envp) {
    (void)argc; (void)argv; (void)envp;
    printf("\n--- Prime Number Benchmark Starting ---\n");
    
    unsigned int start_ticks = get_ticks();
    int count = 0;
    int upper_limit = 1500;
    
    for (int i = 1; i <= upper_limit; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    
    unsigned int end_ticks = get_ticks();
    printf("Found %d primes under %d.\n", count, upper_limit);
    printf("Execution time: %d ticks.\n", end_ticks - start_ticks);
    printf("--- Benchmark Completed ---\n");
    return 0;
}