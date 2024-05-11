#ifndef SHARED_MEM_H 
#define SHARED_MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

#define LOG_SEM_NAME "log_semaphore"

// Pipe Names
#define USER_PIPE "user_pipe"
#define BACK_PIPE "back_pipe"

#define MSQ_FILE "msq_file.txt"

typedef struct program_init{
    int max_mobile_users;
    int queue_pos;
    int auth_servers;
    int auth_proc_time;
    int max_video_wait;
    int max_others_wait; 
} program_init;

// User struct
typedef struct user{
    int initial_plafond;
    int current_plafond; 
    int dados_reservar;
    int id;
    int index;
}user;

// Message queue struct
typedef struct {
    long priority;
    char message[124];
} queue_msg;

// Statistic struct
typedef struct statistic {
    int td_video, ar_video;
    int td_music, ar_music;
    int td_social, ar_social;
}statistic;

// Shared memory struct
typedef struct{
    user *mem;
    int *authorization_free;
    statistic stats;

    //pthread_mutex_t monitor_mutex;
    //pthread_cond_t monitor_cond;
} shared_m;

// Video Queue and Other Queue
typedef struct Node {
    char *command;
    struct Node* next;
}Node;

typedef struct Queue {
    struct Node* front;
    struct Node* rear;
}Queue;

//======================================

// Initial config variable
program_init *config;

// Threads names
pthread_t receiver_thread;
pthread_t sender_thread;
pthread_t back_office_stats, monitor_warning;

// Principal process
pid_t authorization_request_manager_process;
pid_t monitor_engine_process;

// Time Variables
time_t now;
struct tm *t;

// Shared Memory variable
shared_m *shm;
int shm_id;

//Message Queue
int mq_id;

//Semaphore
sem_t *user_sem;
sem_t *autho_free;
sem_t *stats_sem;
sem_t *monitor_sem;

// Queue variables
struct Queue* queue_video;
struct Queue* queue_other;

int running;

#endif