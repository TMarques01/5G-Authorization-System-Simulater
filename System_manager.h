#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

#define USER_SEM "user_sem"
#define AUTH_SEM "authorization_engine_sem"



struct DispatcherArgs {
  int (*pipes)[2];
  struct Queue* queue_video;
  struct Queue* queue_other;
};

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
#endif