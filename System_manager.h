#ifndef SYSTEM_MAN_H
#define SYSTEM_MAN_H

#include "shared_memory.h"

#define USER_SEM "user_sem"
#define AUTH_SEM "authorization_engine_sem"

struct ThreadArgs {
  int (*pipes)[2];
  struct Queue* queue_video;
  struct Queue* queue_other;
};

void create_unnamed_pipes(int pipes[][2]);
int is_dados_reservar_zero(users_list *list, int id);
int user_in_list(users_list *list, int id);
void add_to_dados_reservar(users_list *list, int id, int add_value);
int update_plafond(shared_m *shared_data, int user_id);

// Process and Threads
void monitor_engine();
void *receiver(void *arg);
void *sender(void *arg);
void authorization_request_manager();

// Main program funcions
void cleanup();
void init_log();
void init_program();
#endif