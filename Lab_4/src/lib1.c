#include <math.h>
#include "mathlib.h"

int PrimeCount(int A, int B) {
    int count = 0;
    for (int i = A; i <= B; i++) {
        if (i < 2) continue;
        int is_prime = 1;
        for (int j = 2; j <= i / 2; j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) {
            count++;
        }
    }
    return count;
}

float E(int x) {
    if (x <= 0) return 1.0f;
    return powf(1.0f + 1.0f / (float)x, (float)x);
}