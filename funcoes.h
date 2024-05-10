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
//int remove_user_from_list(int user_id);
int add_user_to_list(user new_user);
int user_in_list(int id);
int is_dados_reservar_zero(int id);
void add_to_dados_reservar(int id, int add_value);
int update_plafond(int id, char *type);
void print_user_list();

#endif