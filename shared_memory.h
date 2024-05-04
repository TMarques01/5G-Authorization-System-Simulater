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

typedef struct program_init{
    int max_mobile_users;
    int queue_pos;
    int auth_servers;
    int auth_proc_time;
    int max_video_wait;
    int max_others_wait; 
} program_init;

// Struct dos users
typedef struct user{
    int initial_plafond;
    int current_plafond; 
    int dados_reservar;
    int id;
}user;

// Lista ligada dos users
typedef struct users_list{
	user user;
	struct users_list * next;
}users_list;

// Shared memory variable
typedef struct{
    users_list *mem;
    int *authorization_free;
} shared_m;

shared_m *shm;
int shm_id;

// Video Queue and Other Queue
typedef struct Node {
    char *command;
    struct Node* next;
}Node;

typedef struct Queue {
    struct Node* front;
    struct Node* rear;
}Queue;

// Queue Pointer
struct Queue* queue_video;
struct Queue* queue_other;

//======================================

// Initial config variable
program_init *config;

// Threads names
pthread_t receiver_thread;
pthread_t sender_thread;

// Principal process
pid_t authorization_request_manager_process;
pid_t monitor_engine_process;

// Time Variables
time_t now;
struct tm *t;

//Message Queue
int mq_id;

//Semaphore
sem_t *user_sem;
sem_t *autho_free;

int running;

#endif