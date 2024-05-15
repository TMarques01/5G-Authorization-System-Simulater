//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

#define USER_SEM "user_sem"
#define AUTH_SEM "authorization_engine_sem"
#define STATS_SEM "statistic_sem"
#define MON_SEM "monitor_sem"
#define RUN_SEM "running_sem"
#define COND_SEM "condition_var"

struct ThreadArgs {
  int (*pipes)[2];
  struct Queue* queue_video;
  struct Queue* queue_other;
};

int (*pipes)[2];

int init_unnamed_pipes();

// Create unnamed pipes
void author_signal(int sig);
void monitor_signal(int sig);

// Queue
struct Queue* createQueue();
char* dequeue(struct Queue* queue, int i);
int queue_size(struct Queue* queue, int i);
void write_Queue(struct Queue* queue);
struct Node* createNode(char *command);
void destroyQueue(struct Queue* queue);
void enqueue(struct Queue* queue, char *command, int i);

// Process and Threads
void authorization_engine(int id);
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void authorization_request_manager();

// Main program funcions
void cleanup(int sig);
void init_log();
void init_program();
#endif