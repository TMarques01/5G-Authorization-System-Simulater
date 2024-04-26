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
    int max_request;
    int video;
    int music;
    int social;
    int dados_reservar;
    int id;
}user;

// Lista ligada dos users
typedef struct users_list{
	struct user user;
	struct users_list * next;
}users_list;

// Message queue struct
typedef struct {
   long priority;
   user current_user;
}video_queue;

typedef struct {
   long priority;
   user current_user;
}other_queue;


// Shared memory variable
users_list *mem;
int shm_id;

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
int mq_v_id;
int mq_o_id;
int mq_id;

//Sem
sem_t *user_list;

#endif