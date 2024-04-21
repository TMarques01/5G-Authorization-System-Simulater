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

#endif
