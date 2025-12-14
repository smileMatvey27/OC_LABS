#include <stdlib.h>
#include <string.h>
#include "mathlib.h"

int PrimeCount(int A, int B) {
    if (B < 2 || A > B) return 0;
    if (A < 2) A = 2; 

    char *is_prime = (char*)malloc((B + 1) * sizeof(char));
    if (!is_prime) return -1; 
    memset(is_prime, 1, (B + 1) * sizeof(char));

    is_prime[0] = is_prime[1] = 0; 

    for (int i = 2; i * i <= B; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= B; j += i) {
                is_prime[j] = 0;
            }
        }
    }

    int count = 0;
    for (int i = A; i <= B; i++) {
        if (is_prime[i]) count++;
    }

    free(is_prime);
    return count;
}

float E(int x) {
    if (x < 0) return 1.0f; 

    float e = 1.0f;
    float factorial = 1.0f;
    for (int n = 1; n <= x; n++) {
        factorial *= n; 
        e += 1.0f / factorial;
    }
    return e;
}