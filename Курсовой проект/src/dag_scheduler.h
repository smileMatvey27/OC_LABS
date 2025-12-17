// dag_scheduler.h
#ifndef DAG_SCHEDULER_H
#define DAG_SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#define MAX_JOBS 100
#define MAX_NAME_LEN 50
#define MAX_CMD_LEN 512
#define MAX_DEPS 10

typedef enum {
    JOB_PENDING,
    JOB_READY,
    JOB_RUNNING,
    JOB_SUCCESS,
    JOB_FAILED
} JobStatus;

typedef struct Job {
    char name[MAX_NAME_LEN];
    char command[MAX_CMD_LEN];
    
    struct Job* dependencies[MAX_DEPS];
    int dep_count;
    
    struct Job* dependents[MAX_DEPS];
    int dep_on_count;
    
    JobStatus status;
    int exit_code;
    pthread_t thread_id;
} Job;

typedef struct {
    Job* jobs[MAX_JOBS];
    int job_count;
    
    pthread_mutex_t mutex;
    bool has_failure;
    bool stop_requested;
    bool all_jobs_completed;  // Добавляем этот флаг
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    Job* ready_queue[MAX_JOBS];
    int queue_size;
    int queue_front;
    int queue_rear;
} DAG;

DAG* dag_create();
void dag_destroy(DAG* dag);
Job* dag_add_job(DAG* dag, const char* name, const char* command);
bool dag_add_dependency(DAG* dag, const char* from, const char* to);
bool dag_validate(DAG* dag);
void dag_execute(DAG* dag);
void dag_stop(DAG* dag);
bool parse_ini(const char* filename, DAG* dag);
void trim_string(char* str);
Job** get_start_jobs(DAG* dag, int* count);
Job** get_end_jobs(DAG* dag, int* count);

#endif