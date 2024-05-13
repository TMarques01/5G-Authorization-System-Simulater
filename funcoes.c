//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "shared_memory.h"
#include "funcoes.h"

// Create Named pipes
void create_named_pipe(char *name){ 
  unlink(name);
  if ((mkfifo(name, O_CREAT|O_EXCL|0600)<0) && (errno != EEXIST)){
    printf("CANNOT CREATE NAMED PIPE -> EXITING\n");
    exit(1);
  }
}

// Function to write in log file
void write_log(char *writing){

    // Sempahore for writing in the file
    sem_wait(log_semaphore);

    // Variables for time
    now = time(NULL);
    t = localtime(&now);

    fprintf(log_file, "%02d:%02d:%02d  %s\n",t->tm_hour, t->tm_min, t->tm_sec, writing);
	printf("%02d:%02d:%02d  %s\n",t->tm_hour, t->tm_min, t->tm_sec, writing);

    // Clean buffer
    fflush(log_file);

    sem_post(log_semaphore);
}

// Function to read from the pipe
char *read_from_pipe(int pipe_fd){

    char buffer[1024];
    ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer));

    if (bytes_read == -1) {
        return NULL;
    } else {
        char* data = malloc(bytes_read + 1);
        memcpy(data, buffer, bytes_read);
        data[bytes_read] = '\0';
        return data;
    }
}

// Verify if a string is a number
int is_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0; // Não é um número
        }
    }
    return 1; // É um número
}

// Verify the values from the file
int file_verification(const char* filename) {
    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        printf("Erro ao abrir o ficheiro\n");
        return -1;
    }

    char line[50];
    int count = 0;
    int temp_val;

    config = malloc(sizeof(program_init));

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        // Verifica se a linha contém um número
        if (is_number(line)) {
            temp_val = atoi(line);
            // Verifica se o valor está dentro dos critérios esperados
            if (temp_val < 0 || ((count == 2 || count == 4 || count == 5) && temp_val < 1)) {
                printf("Valores incorretos no ficheiro\n");
                fclose(f);
                return -2;
            }

            // Atribui o valor lido à estrutura, baseado no contador
            switch (count) {
                case 0: config->max_mobile_users = temp_val; break;
                case 1: config->queue_pos = temp_val; break;
                case 2: config->auth_servers = temp_val; break;
                case 3: config->auth_proc_time = temp_val; break;
                case 4: config->max_video_wait = temp_val; break;
                case 5: config->max_others_wait = temp_val; break;
                default:
                    fclose(f);
                    return -3; // Número inesperado de linhas
            }
            count++;
        } else {
            printf("Linha não é um número válido.\n");
            fclose(f);
            return -2;
        }
    }

    fclose(f);

    if (count != 6) {
        printf("Número insuficiente de dados!\n");
        return -3;
    }

    return 0; // Sucesso
}

/*
// Função para remover um usuário específico da lista pelo ID
int remove_user_from_list(int user_id) {
    users_list *current = shm->mem;
    users_list *previous = NULL;

    if (shm->mem == NULL) {
        return -1; // Lista vazia
    }

    // Procurar o usuário a ser removido
    while (current != NULL && current->user.id != user_id) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        return -1; // Usuário não encontrado
    }

    // Remover o usuário encontrado
    if (previous == NULL) {
        // O usuário a ser removido está na cabeça da lista
        shm->mem = current->next;
    } else {
        // O usuário está em algum lugar após o primeiro elemento
        previous->next = current->next;
    }

    // Libera a memória do nó removido
    free(current);

    return 0; // Sucesso
}
*/

// ============= Linked List Functions =============

// Function to add a user to a list
int add_user_to_list(user new_user) {
    // Criando um novo nó para a lista
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == -999){
            shm->mem[i] = new_user;
            shm->mem[i].index = i;
            sem_post(user_sem);
            return 0;
        }
    }

    sem_post(user_sem);

    return -1;
}

// Function to verify if the user is in the list
int user_in_list(int id) {
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == id){
            
            sem_post(user_sem);
            return 1;
        }
    }

    sem_post(user_sem);
    return 0;  // ID não encontrado na lista
}

// Function to verify if "dados_reservar" is equal to -1
int is_dados_reservar_zero(int id) {
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == id){
            if (shm->mem[i].dados_reservar == -1){
                sem_post(user_sem);
                return 1;
            } else {
                sem_post(user_sem);
                return 0;
            }
        }
    }

    sem_post(user_sem);
    return 0;  // Retorna 0 se o ID não for encontrado
}

// Function to add to "dados_reservar" the value
void add_to_dados_reservar(int id, int add_value) {
    sem_wait(user_sem);

    for (int i = 0; i <config->max_mobile_users; i++){
        if (shm->mem[i].id == id){
            shm->mem[i].dados_reservar = add_value;
            sem_post(user_sem);
            return;
        }
    }

    sem_post(user_sem);
}

// Function to update the plafond
int update_plafond(int id, char *type) {
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == id){
            if (shm->mem[i].current_plafond >= shm->mem[i].dados_reservar){
                shm->mem[i].current_plafond -= shm->mem[i].dados_reservar;

                sem_wait(stats_sem);
                if (strcmp(type, "VIDEO") == 0){
                    shm->stats.td_video += shm->mem[i].dados_reservar;
                } else if (strcmp(type, "SOCIAL") == 0){
                    shm->stats.td_social += shm->mem[i].dados_reservar;
                } else {
                    shm->stats.td_music += shm->mem[i].dados_reservar;
                }
                sem_post(stats_sem);

                sem_post(user_sem);
                return 0;
            } else {
                shm->mem[i].current_plafond = 0;
                sem_post(user_sem);
                return 1;
            }
        }
    }

    sem_post(user_sem);
    return -1;
}

// Print user list
void print_user_list() {
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if ((shm->mem[i].id != -999)){
            printf("ID %d, Inital Plafond: %d, Actual Plafond %d, Reserved Data: %d, Index %d\n", shm->mem[i].id, shm->mem[i].initial_plafond, shm->mem[i].current_plafond, shm->mem[i].dados_reservar, shm->mem[i].index);
        }
    }

    printf("\n");
    sem_post(user_sem);
}

// ================ AUX FUNCTIONS =====================

// Verify how long an orden has been in the queue
int verify_queue_time(double elapsed, char *command){


    char *token = strtok(command, "#");
    token = strtok(NULL, "#");

    if (strcmp(token, "VIDEO") == 0){
        if (elapsed > config->max_video_wait/1000){
            return 1;
        } else {
            return 0;
        }
    } else {
        if (elapsed > config->max_others_wait/1000){
            return 1;
        } else {
            return 0;
        }
    }
}

// Creat Unnamed pipes
void create_unnamed_pipes(int pipes[][2]){
  for (int i = 0; i < config->auth_servers; i++) {
        if (pipe(pipes[i]) == -1) {
          write_log("CANNOT CREATE UNNAMED PIPE -> EXITING");
          exit(1);
        }
    }
}

// Verify wich AE is free
int check_authorization_free(int i){

    sem_wait(autho_free);
    if (shm->authorization_free[i] == 0){
        shm->authorization_free[i] = 1;

        #ifdef ARRAY
        for (int i = 0; i < config->auth_servers + 1; i++) printf("AUTHORIZATION_FREE[%d]: %d \n", i + 1, shm->authorization_free[i]);
        #endif
        
        sem_post(autho_free);
        return 1;
    } else {
        sem_post(autho_free);
        return 0;
    }
}

// Verify if the user list is full
int verify_user_list_full(){
    sem_wait(user_sem);
    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == -999){
            sem_post(user_sem);
            return 0;
        } else {
            continue;
        }
    }
    sem_post(user_sem);
    return 1;
}

int verify_running(){
    sem_wait(running_sem);
    if (shm->running == 1){
        sem_post(running_sem);
        return 1;
    } else {
        sem_post(running_sem);
        return 0;
    }
}