#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

#define USER_SEM "user_sem"
#define AUTH_SEM "authorization_engine_sem"
#define STATS_SEM "statistic_sem"
#define MON_SEM "monitor_sem"

struct ThreadArgs {
  int (*pipes)[2];
  struct Queue* queue_video;
  struct Queue* queue_other;
};

void create_unnamed_pipes(int pipes[][2]);
int check_authorization_free(int i);
int verify_user_list_full();

// Queue
struct Queue* createQueue();
char* dequeue(struct Queue* queue, int i);
int queue_size(struct Queue* queue, int i);
void printQueue(struct Queue* queue, int i);
void write_Queue(struct Queue* queue);
struct Node* createNode(char *command);
void destroyQueue(struct Queue* queue);
void enqueue(struct Queue* queue, char *command, int i);

// User´s List
int is_dados_reservar_zero(int id);
int user_in_list(int id);
void add_to_dados_reservar(int id, int add_value);
int update_plafond(int id, char *type);
int add_user_to_list(user new_user);
void print_user_list();

// Process and Threads
void authorization_engine(int id, int (*pipes)[2]);
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void authorization_request_manager();

// Main program funcions
void cleanup();
void init_log();
void init_program();
#endif