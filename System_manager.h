#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

#define USER_SEM "user_sem"

// Process and Threads
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void authorization_request_manager();
int is_dados_reservar_zero(users_list *list, int id);
int user_in_list(users_list *list, int id);
void add_to_dados_reservar(users_list *list, int id, int add_value);

// Main program funcions
void cleanup();
void init_log();
void init_program();
/*
// Auxiliar Functions
void create_named_pipe(char *name);
int file_verification(const char* filename);
int is_number(char* str);
void write_log(char *writing);
char *read_from_pipe(int pipe_fd);
*/
#endif