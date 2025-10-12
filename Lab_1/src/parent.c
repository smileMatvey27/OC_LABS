#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

int main() {
    char filename[256];
    
    printf("Введите имя файла: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        perror("Ошибка чтения имени файла");
        exit(1);
    }
    
    filename[strcspn(filename, "\n")] = 0;
    
    if (access(filename, F_OK) != 0) {
        printf("Ошибка: файл '%s' не существует\n", filename);
        exit(1);
    }
    
    FILE *test_file = fopen(filename, "r");
    if (test_file == NULL) {
        perror("Ошибка открытия файла");
        exit(1);
    }
    
    int is_empty = 1;
    int ch;
    while ((ch = fgetc(test_file)) != EOF) {
        if (!isspace(ch)) {
            is_empty = 0;
            break;
        }
    }
    fclose(test_file);
    
    if (is_empty) {
        printf("Ошибка: файл '%s' пустой\n", filename);
        exit(1);
    }
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Ошибка создания pipe");
        exit(1);
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Ошибка fork");
        exit(1);
    }
    
    if (pid == 0) {
        close(pipefd[0]);
        
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("Ошибка dup2 в дочернем процессе");
            exit(1);
        }
        close(pipefd[1]);
        
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("Ошибка открытия файла");
            exit(1);
        }
        
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("Ошибка dup2 для файла");
            close(fd);
            exit(1);
        }
        close(fd);
        
        execl("./child", "child", NULL);
        perror("Ошибка execl");
        exit(1);
    } else {
        close(pipefd[1]);
        
        char buffer[1024];
        ssize_t bytes_read;
        
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
        close(pipefd[0]);
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 2) {
                printf("Программа завершена из-за ошибки деления на ноль или в строке присутствуют другие символы, не являющиеся числами.\n");
                exit(1);
            } else if (exit_status != 0) {
                exit(1);
            }
        }
        
        printf("Все вычисления завершены успешно.\n");
    }
    
    return 0;
}