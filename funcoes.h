#ifndef FUNCOES_H
#define FUNCOES_H

#include "System_manager.h"
#include "shared_memory.h"

//Log file management
FILE *log_file;
sem_t *log_semaphore;


// Protótipos das funções definidas em funcoes.c
void create_named_pipe(char *name);
int file_verification(const char* filename);
int is_number(char* str);
void write_log(char *writing);
char *read_from_pipe(int pipe_fd);
void add_user_to_list(user new_user);
int remove_user_from_list(int user_id);
void print_user_list();
void create_unnamed_pipes(int pipes[][2]);


struct Queue* createQueue();
int isEmpty(struct Queue* queue);
char* dequeue(struct Queue* queue);
int queue_size(struct Queue* queue);
void printQueue(struct Queue* queue);
void write_Queue(struct Queue* queue);
struct Node* createNode(char *command);
void destroyQueue(struct Queue* queue);
void enqueue(struct Queue* queue, char *command);

#endif
