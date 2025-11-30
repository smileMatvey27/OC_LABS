#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define SHM_NAME "/lab3_shm"
#define MAX_LINE_LENGTH 1024

typedef struct {
    int stop_flag;
    char line[MAX_LINE_LENGTH];
    float result;
    int result_ready;
} shared_data_t;

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        handle_error("Error opening file");
    }

    // Создаем shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        handle_error("Error creating shared memory");
    }

    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        handle_error("Error setting shared memory size");
    }

    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), 
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        handle_error("Error mapping shared memory");
    }

    // Инициализируем shared data
    shared_data->stop_flag = 0;
    shared_data->line[0] = '\0';
    shared_data->result = 0.0f;
    shared_data->result_ready = 0;

    // Создаем дочерний процесс
    pid_t pid = fork();
    
    if (pid == -1) {
        handle_error("Error creating process");
    }

    if (pid == 0) {
        // Дочерний процесс
        execl("./a.out", "a.out", NULL);
        handle_error("Error starting child process");
    } else {
        // Родительский процесс
        char line[MAX_LINE_LENGTH];
        
        while (fgets(line, sizeof(line), file) != NULL) {
            if (shared_data->stop_flag) {
                break;
            }
            
            line[strcspn(line, "\n")] = '\0';
            
            if (strlen(line) == 0) {
                continue;
            }
            
            strcpy(shared_data->line, line);
            
            while (!shared_data->result_ready && !shared_data->stop_flag) {
                usleep(10000);
            }
            
            if (shared_data->stop_flag) {
                break;
            }
            
            shared_data->result_ready = 0;
        }
        
        fclose(file);

        if (!shared_data->stop_flag) {
            shared_data->stop_flag = 1;
            wait(NULL);
        }

        munmap(shared_data, sizeof(shared_data_t));
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
    
    return 0;
}