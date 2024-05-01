//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "shared_memory.h"
#include "funcoes.h"

// Creat Uname pipes
void create_unnamed_pipes(int pipes[][2]){
  for (int i = 0; i < config->auth_servers; i++) {
        if (pipe(pipes[i]) == -1) {
          print("CANNOT CREATE UNNAMED PIPE -> EXITING\n");
          exit(1);
        }
    }
}

// Creat Named pipes
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

//Função para verificar se uma string é um número
int is_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0; // Não é um número
        }
    }
    return 1; // É um número
}

//Função de verificação do ficheiro
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

// Função para adicionar um usuário à lista ligada, recebendo diretamente uma struct user
void add_user_to_list(user new_user) {
    // Criando um novo nó para a lista
    users_list *new_node = (users_list *)malloc(sizeof(users_list));
    if (new_node == NULL) {
        fprintf(stderr, "Failed to allocate memory for new user node\n");
        return;
    }

    // Inicializando o novo nó com os dados fornecidos
    new_node->user = new_user;
    new_node->next = NULL;

    // Adicionando o novo nó à lista ligada
    if (mem == NULL) {
        // A lista está vazia, o novo nó se torna a cabeça da lista
        mem = new_node;
    } else {
        // Encontrar o final da lista e adicionar o novo nó
        users_list *current = mem;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Função para remover um usuário específico da lista pelo ID
int remove_user_from_list(int user_id) {
    users_list *current = mem;
    users_list *previous = NULL;

    if (mem == NULL) {
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
        mem = current->next;
    } else {
        // O usuário está em algum lugar após o primeiro elemento
        previous->next = current->next;
    }

    // Libera a memória do nó removido
    free(current);

    return 0; // Sucesso
}

// Print user list
void print_user_list() {
    users_list *current = mem;
    current = current->next;
    printf("Users in the list:\n");
    while (current != NULL) {
        printf("ID: %d, Plafond: %d, Reserved Data: %d\n",
               current->user.id, current->user.initial_plafond,current->user.dados_reservar);
        current = current->next;
    }
}