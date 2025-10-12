#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main() {
    char line[1024];
    int empty_file = 1; 
    
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) continue;
        
        empty_file = 0; 
        
        float *numbers = NULL;
        int capacity = 10;
        int count = 0;
        
        numbers = (float*)malloc(capacity * sizeof(float));
        if (numbers == NULL) {
            fprintf(stdout, "Ошибка выделения памяти\n"); 
            exit(1);
        }
        
        char *token = strtok(line, " ");
        while (token != NULL) {
            if (count >= capacity) {
                capacity *= 2;
                float *temp = (float*)realloc(numbers, capacity * sizeof(float));
                if (temp == NULL) {
                    fprintf(stdout, "Ошибка перевыделения памяти\n");
                    free(numbers);
                    exit(1);
                }
                numbers = temp;
            }
            
            numbers[count++] = atof(token);
            token = strtok(NULL, " ");
        }
        
        if (count < 2) {
            fprintf(stdout, "Ошибка: в строке должно быть хотя бы 2 числа\n");
            fflush(stdout);
            free(numbers);
            continue;
        }
        
        for (int i = 1; i < count; i++) {
            if (numbers[i] == 0.0f) {
                free(numbers);
                exit(2); 
            }
        }
        
        float result = numbers[0];
        for (int i = 1; i < count; i++) {
            result /= numbers[i];
        }
        
        printf("%f\n", result);
        fflush(stdout);
        
        free(numbers);
    }
    
    if (empty_file) {
        fprintf(stdout, "Ошибка: файл пустой\n"); 
        exit(1);
    }
    
    return 0;
}