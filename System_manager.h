#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

// Linked List Functions
users_list* create_user_node(user userdata); 
int is_list_empty(users_list* head);
void add_list_user(users_list** head, user userdata);
void print_user_list();
void destroy_user_list();

// Process and Threads
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void authorization_request_manager();

// Main program funcions
void cleanup();
void init_log();
void init_program();

// Auxiliar Functions
void create_named_pipe(char *name);
int file_verification(const char* filename);
int is_number(char* str);
void write_log(char *writing);
char *read_from_pipe(int pipe_fd);

#endif