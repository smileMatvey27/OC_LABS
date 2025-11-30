#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define SHM_NAME "Local\\DivCalcSharedMemory"
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
    HANDLE hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHM_NAME);

    if (hMapFile == NULL) {
        return 1;
    }

    shared_data_t *shared_data = (shared_data_t *)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(shared_data_t));

    if (shared_data == NULL) {
        CloseHandle(hMapFile);
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
        
        Sleep(10);
    }

    UnmapViewOfFile(shared_data);
    CloseHandle(hMapFile);
    
    return 0;
}