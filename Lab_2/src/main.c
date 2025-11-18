#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

int *array, size, max_threads;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int phase;
    int thread_id;
} ThreadData;

void swap(int i, int j) {
    int tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}

void* worker(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    int start = data->thread_id * 2 + data->phase;
    int step = max_threads * 2;
    
    for (int i = start; i < size - 1; i += step) {
        if (array[i] > array[i + 1]) {
            pthread_mutex_lock(&mutex);
            swap(i, i + 1);
            pthread_mutex_unlock(&mutex);
        }
    }
    
    free(data);
    return NULL;
}

int is_sorted() {
    for (int i = 0; i < size - 1; i++)
        if (array[i] > array[i + 1]) return 0;
    return 1;
}

void print_array(int *arr, char *label) {
    printf("%s: ", label);
    for (int i = 0; i < size; i++) {
        printf("%d", arr[i]);
        if (i < size - 1) printf(" ");
    }
    printf("\n");
}

void parallel_sort() {
    pthread_t *threads = malloc(max_threads * sizeof(pthread_t));
    int sorted = 0;
    int iterations = 0;
    
    while (!sorted && iterations < size) {
        // Even phase
        for (int i = 0; i < max_threads; i++) {
            ThreadData *data = malloc(sizeof(ThreadData));
            data->phase = 0;
            data->thread_id = i;
            pthread_create(&threads[i], NULL, worker, data);
        }
        for (int i = 0; i < max_threads; i++) pthread_join(threads[i], NULL);
        
        // Odd phase
        for (int i = 0; i < max_threads; i++) {
            ThreadData *data = malloc(sizeof(ThreadData));
            data->phase = 1;
            data->thread_id = i;
            pthread_create(&threads[i], NULL, worker, data);
        }
        for (int i = 0; i < max_threads; i++) pthread_join(threads[i], NULL);
        
        sorted = is_sorted();
        iterations++;
    }
    
    printf("Sorting completed in %d iterations\n", iterations);
    free(threads);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <size> <threads>\n", argv[0]);
        return 1;
    }
    
    size = atoi(argv[1]);
    max_threads = atoi(argv[2]);
    
    if (max_threads > size/2) max_threads = size/2;
    if (max_threads < 1) max_threads = 1;
    
    array = malloc(size * sizeof(int));
    int *original = malloc(size * sizeof(int));
    
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 100;
        original[i] = array[i];
    }
    
    printf("=== PARALLEL BATCHER ODD-EVEN SORT ===\n");
    printf("Array size: %d, Threads: %d\n\n", size, max_threads);
    
    print_array(original, "Unsorted array");
    
    parallel_sort();
    
    print_array(array, "Sorted array  ");
    printf("Verification: %s\n", is_sorted() ? "SORTED CORRECTLY" : "NOT SORTED");
    
    free(array);
    free(original);
    pthread_mutex_destroy(&mutex);
    return 0;
}