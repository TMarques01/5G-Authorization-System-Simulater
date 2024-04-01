#ifndef shared_mem 
#define shared_mem

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#define LOG_SEM_NAME "log_semaphore"

typedef struct program_init{
    int max_mobile_users;
    int queue_pos;
    int auth_servers;
    int auth_proc_time;
    int max_video_wait;
    int max_others_wait; 
} program_init;

typedef struct user{
    int initial_plafond, max_request, video, music, social, dados_reservar;
}user;

typedef struct users_list{
	struct user user;
}users_list;

users_list *mem;
int shmid;

// Initial config variable
program_init *config;

//Log file management
FILE *log_file;
sem_t *log_semaphore;

// Threads names
pthread_t receiver_thread;
pthread_t sender_thread;

// Principal process
pid_t authorization_request_manager_process;
pid_t monitor_engine_process;

//
time_t now;
struct tm *t;

#endif