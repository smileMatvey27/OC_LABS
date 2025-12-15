#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#define SHM_NAME "/lab3_shm"
#define MAX_LINE_LENGTH 1024
#define MAX_NUMBERS 100

typedef struct {
    int stop_flag;
    char line[MAX_LINE_LENGTH];
    float result;
    int result_ready;
} shared_data_t;

int parse_numbers(const char *line, float *numbers, int max_count) {
    char *token;
    char line_copy[MAX_LINE_LENGTH];
    int count = 0;
    
    strcpy(line_copy, line);
    token = strtok(line_copy, " \t\n");
    
    while (token != NULL && count < max_count) {
        numbers[count++] = atof(token);
        token = strtok(NULL, " \t\n");
    }
    
    return count;
}

int main() {
    // Открыть разделяемую память POSIX
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }
    
    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t),
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        return 1;
    }
    
    float numbers[MAX_NUMBERS];
    
    while (1) {
        if (shared_data->stop_flag) {
            break;
        }
        
        if (strlen(shared_data->line) > 0) {
            int num_count = parse_numbers(shared_data->line, numbers, MAX_NUMBERS);
            
            if (num_count >= 2) {
                float result = numbers[0];
                int error = 0;
                
                printf("Calculation: %.2f", numbers[0]);
                for (int i = 1; i < num_count; i++) {
                    printf(" / %.2f", numbers[i]);
                    
                    if (numbers[i] == 0.0f) {
                        printf(" -> ERROR: division by zero!\n");
                        shared_data->stop_flag = 1;
                        error = 1;
                        break;
                    }
                    result /= numbers[i];
                }
                
                if (!error) {
                    printf(" = %.2f\n", result);
                    shared_data->result = result;
                    shared_data->result_ready = 1;
                }
            }
            
            shared_data->line[0] = '\0';
        }
        
        usleep(10000);  // 10 ms
    }
    
    munmap(shared_data, sizeof(shared_data_t));
    close(shm_fd);
    
    return 0;
}