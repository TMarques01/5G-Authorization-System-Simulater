#ifndef System_manager
#define System_manager

#include "shared_memory.h"

// Pipe Names
#define USER_PIPE "user_pipe"
#define BACK_PIPE "back_pipe"

//Log file management
FILE *log_file;
sem_t *log_semaphore;

void write_log(char *writing);
int is_number(char* str);
int file_verification(const char* filename);
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void create_named_pipe(char *name);
void authorization_request_manager();
void cleanup();
void init_log();
void init_program();

#endif