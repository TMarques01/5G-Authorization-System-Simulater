//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#ifndef FUNCOES_H
#define FUNCOES_H

#include "System_manager.h"
#include "shared_memory.h"

//Log file management
FILE *log_file;
sem_t *log_semaphore;


// Protótipos das funções definidas em funcoes.c
void write_log(char *writing);

void create_named_pipe(char *name);
char *read_from_pipe(int pipe_fd);
void create_unnamed_pipes(int pipes[][2]);

int file_verification(const char* filename);
int is_number(char* str);
int verify_queue_time(double elapsed, char *command);
int check_authorization_free(int i);
int verify_user_list_full();
int verify_running();


//============= Linked List Functions =============

//int remove_user_from_list(int user_id);
int add_user_to_list(user new_user);
int user_in_list(int id);
int is_dados_reservar_zero(int id);
void add_to_dados_reservar(int id, int add_value);
int update_plafond(int id, char *type);
void print_user_list();

// ===============================================



#endif