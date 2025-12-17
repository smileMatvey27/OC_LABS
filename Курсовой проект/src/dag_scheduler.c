#include "dag_scheduler.h"

DAG* dag_create() {
    DAG* dag = (DAG*)malloc(sizeof(DAG));
    if (!dag) return NULL;
    
    dag->job_count = 0;
    dag->has_failure = false;
    dag->stop_requested = false;
    dag->all_jobs_completed = false;
    dag->queue_size = 0;
    dag->queue_front = 0;
    dag->queue_rear = 0;
    
    pthread_mutex_init(&dag->mutex, NULL);
    pthread_mutex_init(&dag->queue_mutex, NULL);
    pthread_cond_init(&dag->queue_cond, NULL);
    
    return dag;
}

void dag_destroy(DAG* dag) {
    if (!dag) return;
    
    for (int i = 0; i < dag->job_count; i++) {
        free(dag->jobs[i]);
    }
    
    pthread_mutex_destroy(&dag->mutex);
    pthread_mutex_destroy(&dag->queue_mutex);
    pthread_cond_destroy(&dag->queue_cond);
    
    free(dag);
}

Job* dag_add_job(DAG* dag, const char* name, const char* command) {
    if (!dag || dag->job_count >= MAX_JOBS) return NULL;
    
    Job* job = (Job*)malloc(sizeof(Job));
    if (!job) return NULL;
    
    strncpy(job->name, name, MAX_NAME_LEN - 1);
    strncpy(job->command, command, MAX_CMD_LEN - 1);
    job->name[MAX_NAME_LEN - 1] = '\0';
    job->command[MAX_CMD_LEN - 1] = '\0';
    
    job->dep_count = 0;
    job->dep_on_count = 0;
    job->status = JOB_PENDING;
    job->exit_code = 0;
    job->thread_id = 0;
    
    dag->jobs[dag->job_count++] = job;
    return job;
}

Job* find_job(DAG* dag, const char* name) {
    for (int i = 0; i < dag->job_count; i++) {
        if (strcmp(dag->jobs[i]->name, name) == 0) {
            return dag->jobs[i];
        }
    }
    return NULL;
}

bool dag_add_dependency(DAG* dag, const char* from, const char* to) {
    Job* job_from = find_job(dag, from);
    Job* job_to = find_job(dag, to);
    
    if (!job_from || !job_to) {
        fprintf(stderr, "Ошибка: Джоб '%s' или '%s' не найден\n", from, to);
        return false;
    }
    
    if (job_to->dep_count >= MAX_DEPS || job_from->dep_on_count >= MAX_DEPS) {
        fprintf(stderr, "Ошибка: Слишком много зависимостей для джоба\n");
        return false;
    }
    
    job_to->dependencies[job_to->dep_count++] = job_from;
    job_from->dependents[job_from->dep_on_count++] = job_to;
    
    return true;
}

static int get_job_index(DAG* dag, Job* job) {
    if (!dag || !job) return -1;
    
    for (int i = 0; i < dag->job_count; i++) {
        if (dag->jobs[i] == job) {
            return i;
        }
    }
    return -1;
}

static bool has_cycle_dfs(DAG* dag, Job* job, bool* visited, bool* rec_stack) {
    int index = get_job_index(dag, job);
    if (index == -1) return false;
    
    if (rec_stack[index]) return true;
    if (visited[index]) return false;
    
    visited[index] = true;
    rec_stack[index] = true;
    
    for (int i = 0; i < job->dep_on_count; i++) {
        if (has_cycle_dfs(dag, job->dependents[i], visited, rec_stack)) {
            return true;
        }
    }
    
    rec_stack[index] = false;
    return false;
}

static void dfs_connectivity(DAG* dag, Job* job, bool* visited) {
    int index = get_job_index(dag, job);
    if (index == -1 || visited[index]) return;
    
    visited[index] = true;
    
    for (int i = 0; i < job->dep_count; i++) {
        dfs_connectivity(dag, job->dependencies[i], visited);
    }
    
    for (int i = 0; i < job->dep_on_count; i++) {
        dfs_connectivity(dag, job->dependents[i], visited);
    }
}

bool dag_validate(DAG* dag) {
    pthread_mutex_lock(&dag->mutex);
    
    if (dag->job_count == 0) {
        printf("DAG пуст\n");
        pthread_mutex_unlock(&dag->mutex);
        return true;
    }
    
    bool* visited = (bool*)calloc(dag->job_count, sizeof(bool));
    bool* rec_stack = (bool*)calloc(dag->job_count, sizeof(bool));
    bool has_cycle = false;
    
    for (int i = 0; i < dag->job_count; i++) {
        memset(visited, 0, dag->job_count * sizeof(bool));
        memset(rec_stack, 0, dag->job_count * sizeof(bool));
        if (has_cycle_dfs(dag, dag->jobs[i], visited, rec_stack)) {
            has_cycle = true;
            break;
        }
    }
    
    free(visited);
    free(rec_stack);
    
    if (has_cycle) {
        fprintf(stderr, "Обнаружен цикл в DAG!\n");
        pthread_mutex_unlock(&dag->mutex);
        return false;
    }
    
    visited = (bool*)calloc(dag->job_count, sizeof(bool));
    dfs_connectivity(dag, dag->jobs[0], visited);
    
    int visited_count = 0;
    for (int i = 0; i < dag->job_count; i++) {
        if (visited[i]) visited_count++;
    }
    
    free(visited);
    
    if (visited_count != dag->job_count) {
        fprintf(stderr, "DAG имеет более одной компоненты связности!\n");
        pthread_mutex_unlock(&dag->mutex);
        return false;
    }
    
    int start_count, end_count;
    Job** start_jobs = get_start_jobs(dag, &start_count);
    Job** end_jobs = get_end_jobs(dag, &end_count);
    
    if (start_count == 0) {
        fprintf(stderr, "Нет стартовых джобов!\n");
        free(start_jobs);
        free(end_jobs);
        pthread_mutex_unlock(&dag->mutex);
        return false;
    }
    
    if (end_count == 0) {
        fprintf(stderr, "Нет завершающих джобов!\n");
        free(start_jobs);
        free(end_jobs);
        pthread_mutex_unlock(&dag->mutex);
        return false;
    }
    
    printf("DAG корректен. Стартовых джобов: %d, Завершающих: %d\n", 
           start_count, end_count);
    
    free(start_jobs);
    free(end_jobs);
    pthread_mutex_unlock(&dag->mutex);
    return true;
}

Job** get_start_jobs(DAG* dag, int* count) {
    Job** start_jobs = (Job**)malloc(dag->job_count * sizeof(Job*));
    *count = 0;
    
    for (int i = 0; i < dag->job_count; i++) {
        if (dag->jobs[i]->dep_count == 0) {
            start_jobs[(*count)++] = dag->jobs[i];
        }
    }
    
    return start_jobs;
}

Job** get_end_jobs(DAG* dag, int* count) {
    Job** end_jobs = (Job**)malloc(dag->job_count * sizeof(Job*));
    *count = 0;
    
    for (int i = 0; i < dag->job_count; i++) {
        if (dag->jobs[i]->dep_on_count == 0) {
            end_jobs[(*count)++] = dag->jobs[i];
        }
    }
    
    return end_jobs;
}

static void enqueue_job(DAG* dag, Job* job) {
    pthread_mutex_lock(&dag->queue_mutex);
    
    if (dag->queue_size < MAX_JOBS) {
        dag->ready_queue[dag->queue_rear] = job;
        dag->queue_rear = (dag->queue_rear + 1) % MAX_JOBS;
        dag->queue_size++;
    }
    
    pthread_mutex_unlock(&dag->queue_mutex);
    pthread_cond_signal(&dag->queue_cond);
}

static Job* dequeue_job(DAG* dag) {
    pthread_mutex_lock(&dag->queue_mutex);
    
    while (dag->queue_size == 0 && 
           !dag->stop_requested && 
           !dag->all_jobs_completed) {
        pthread_cond_wait(&dag->queue_cond, &dag->queue_mutex);
    }
    
    if (dag->stop_requested || 
        (dag->queue_size == 0 && dag->all_jobs_completed)) {
        pthread_mutex_unlock(&dag->queue_mutex);
        return NULL;
    }
    
    Job* job = dag->ready_queue[dag->queue_front];
    dag->queue_front = (dag->queue_front + 1) % MAX_JOBS;
    dag->queue_size--;
    
    pthread_mutex_unlock(&dag->queue_mutex);
    return job;
}

static void execute_job(Job* job, DAG* dag) {
    printf("[%s] Запуск: %s\n", job->name, job->command);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        
        pthread_mutex_lock(&dag->mutex);
        job->status = JOB_FAILED;
        job->exit_code = -1;
        dag->has_failure = true;
        dag_stop(dag);  
        pthread_mutex_unlock(&dag->mutex);
        
        return;
    }
    else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", job->command, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        
        pthread_mutex_lock(&dag->mutex);
        
        if (WIFEXITED(status)) {
            job->exit_code = WEXITSTATUS(status);
            if (job->exit_code == 0) {
                job->status = JOB_SUCCESS;
                printf("[%s] Успешно завершен\n", job->name);
            } else {
                job->status = JOB_FAILED;
                printf("[%s] Завершен с ошибкой (код: %d)\n", job->name, job->exit_code);
                dag->has_failure = true;
                dag_stop(dag);
            }
        } else {
            job->status = JOB_FAILED;
            job->exit_code = -1;
            printf("[%s] Завершен аварийно\n", job->name);
            dag->has_failure = true;
            dag_stop(dag);  
        }
        
        pthread_mutex_unlock(&dag->mutex);
    }
}

static void* worker_thread(void* arg) {
    DAG* dag = (DAG*)arg;
    
    while (1) {
        Job* job = dequeue_job(dag);
        if (!job) break;
        
        pthread_mutex_lock(&dag->mutex);
        if (dag->has_failure || dag->stop_requested) {
            pthread_mutex_unlock(&dag->mutex);
            break;
        }
        job->status = JOB_RUNNING;
        pthread_mutex_unlock(&dag->mutex);
        
        execute_job(job, dag);
        
        pthread_mutex_lock(&dag->mutex);
        if (job->status == JOB_SUCCESS && !dag->has_failure) {
            for (int i = 0; i < job->dep_on_count; i++) {
                Job* dependent = job->dependents[i];
                
                bool all_deps_done = true;
                for (int j = 0; j < dependent->dep_count; j++) {
                    if (dependent->dependencies[j]->status != JOB_SUCCESS) {
                        all_deps_done = false;
                        break;
                    }
                }
                
                if (all_deps_done && dependent->status == JOB_PENDING) {
                    dependent->status = JOB_READY;
                    enqueue_job(dag, dependent);
                }
            }
        }
        
        bool all_completed = true;
        for (int i = 0; i < dag->job_count; i++) {
            if (dag->jobs[i]->status != JOB_SUCCESS && 
                dag->jobs[i]->status != JOB_FAILED) {
                all_completed = false;
                break;
            }
        }
        
        if (all_completed) {
            dag->all_jobs_completed = true;
            pthread_mutex_lock(&dag->queue_mutex);
            pthread_cond_broadcast(&dag->queue_cond);
            pthread_mutex_unlock(&dag->queue_mutex);
        }
        
        pthread_mutex_unlock(&dag->mutex);
    }
    
    return NULL;
}

void dag_stop(DAG* dag) {
    if (!dag) return;
    
    pthread_mutex_lock(&dag->queue_mutex);
    dag->stop_requested = true;
    pthread_cond_broadcast(&dag->queue_cond);
    pthread_mutex_unlock(&dag->queue_mutex);
}

void dag_execute(DAG* dag) {
    if (!dag_validate(dag)) {
        fprintf(stderr, "Невозможно выполнить некорректный DAG\n");
        return;
    }
    
    printf("Запуск планировщика DAG...\n");
    
    int start_count;
    Job** start_jobs = get_start_jobs(dag, &start_count);
    
    for (int i = 0; i < start_count; i++) {
        start_jobs[i]->status = JOB_READY;
        enqueue_job(dag, start_jobs[i]);
    }
    free(start_jobs);
    
    int num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_workers < 1) num_workers = 2;
    
    printf("Используется %d рабочих потоков\n", num_workers);
    
    pthread_t* workers = (pthread_t*)malloc(num_workers * sizeof(pthread_t));
    
    for (int i = 0; i < num_workers; i++) {
        pthread_create(&workers[i], NULL, worker_thread, dag);
    }
    
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }
    
    free(workers);
    
    printf("\nСтатистика выполнения:\n");
    printf("======================\n");
    
    int success_count = 0, failed_count = 0, pending_count = 0;
    
    for (int i = 0; i < dag->job_count; i++) {
        switch (dag->jobs[i]->status) {
            case JOB_SUCCESS:
                success_count++;
                printf("[%s] ✓ Успешно\n", dag->jobs[i]->name);
                break;
            case JOB_FAILED:
                failed_count++;
                printf("[%s] ✗ Ошибка (код: %d)\n", 
                       dag->jobs[i]->name, dag->jobs[i]->exit_code);
                break;
            case JOB_PENDING:
                pending_count++;
                printf("[%s] ○ Не выполнен\n", dag->jobs[i]->name);
                break;
            default:
                printf("[%s] ? Неизвестный статус\n", dag->jobs[i]->name);
        }
    }
    
    printf("\nИтого: %d успешно, %d с ошибкой, %d не выполнено\n",
           success_count, failed_count, pending_count);
    
    if (dag->has_failure) {
        printf("\nDAG остановлен из-за ошибки в одном из джобов!\n");
    }
}

void trim_string(char* str) {
    if (!str) return;
    
    char* start = str;
    while (isspace((unsigned char)*start)) start++;
    
    char* end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    memmove(str, start, end - start + 1);
    str[end - start + 1] = '\0';
}

bool parse_ini(const char* filename, DAG* dag) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Не могу открыть файл: %s\n", filename);
        return false;
    }
    
    char line[512];
    char current_section[50] = "";
    
    char job_names[MAX_JOBS][MAX_NAME_LEN];
    char job_commands[MAX_JOBS][MAX_CMD_LEN];
    int job_count = 0;
    
    char dependencies[MAX_JOBS][MAX_DEPS][MAX_NAME_LEN];
    int dep_counts[MAX_JOBS] = {0};
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        trim_string(line);
        
        if (strlen(line) == 0 || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        if (line[0] == '[' && strchr(line, ']')) {
            char* end_bracket = strchr(line, ']');
            *end_bracket = '\0';
            strcpy(current_section, line + 1);
            continue;
        }
        
        char* equals = strchr(line, '=');
        if (equals) {
            *equals = '\0';
            char* key = line;
            char* value = equals + 1;
            
            trim_string(key);
            trim_string(value);
            
            if (strcmp(current_section, "jobs") == 0) {
                if (job_count < MAX_JOBS) {
                    strcpy(job_names[job_count], key);
                    strcpy(job_commands[job_count], value);
                    job_count++;
                }
            }
            else if (strcmp(current_section, "dependencies") == 0) {
                int job_index = -1;
                for (int i = 0; i < job_count; i++) {
                    if (strcmp(job_names[i], key) == 0) {
                        job_index = i;
                        break;
                    }
                }
                
                if (job_index != -1 && dep_counts[job_index] < MAX_DEPS) {
                    char* dep = strtok(value, ",");
                    while (dep != NULL && dep_counts[job_index] < MAX_DEPS) {
                        trim_string(dep);
                        strcpy(dependencies[job_index][dep_counts[job_index]], dep);
                        dep_counts[job_index]++;
                        dep = strtok(NULL, ",");
                    }
                }
            }
        }
    }
    
    fclose(file);
    
    for (int i = 0; i < job_count; i++) {
        if (!dag_add_job(dag, job_names[i], job_commands[i])) {
            fprintf(stderr, "Ошибка добавления джоба: %s\n", job_names[i]);
            return false;
        }
    }
    
    for (int i = 0; i < job_count; i++) {
        for (int j = 0; j < dep_counts[i]; j++) {
            if (!dag_add_dependency(dag, dependencies[i][j], job_names[i])) {
                fprintf(stderr, "Ошибка добавления зависимости: %s -> %s\n",
                       dependencies[i][j], job_names[i]);
                return false;
            }
        }
    }
    
    printf("Успешно загружено %d джобов из %s\n", job_count, filename);
    return true;
}