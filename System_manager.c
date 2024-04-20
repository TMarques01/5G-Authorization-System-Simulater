//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"

//Log file management
FILE *log_file;
sem_t *log_semaphore;

// ============= USERS LIST FUNCTIONS =============
 
// Function to create a new user node
users_list* create_user_node(user userdata) {
    users_list* newNode = (users_list*) malloc(sizeof(users_list));
    if (newNode == NULL) {
        fprintf(stderr, "Failed to allocate memory for new user node.\n");
        exit(EXIT_FAILURE);
    }
    newNode->user = userdata; // Copia os dados do usuário
    newNode->next = NULL;
    return newNode;
}

// Function to check if the list is empty
int is_list_empty(users_list* head) {
    return (head == NULL);
}

// Function to add a user to the end of the list
void add_list_user(users_list** head, user userdata) {
    users_list* newNode = create_user_node(userdata);
    if (is_list_empty(*head)) {
        *head = newNode;
        return;
    }
    users_list* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = newNode;
}

// Functions to print users list
void print_user_list() {
    if (mem == NULL) {
        printf("The user list is empty.\n");
        return;
    }

    users_list *current = mem;
    printf("User List:\n");
    while (current != NULL) {
        printf("ID: %d, Plafond: %d, Max Requests: %d, Video: %d, Music: %d, Social: %d, Reserved Data: %d\n",
            current->user.id, current->user.initial_plafond, current->user.max_request, current->user.video,
            current->user.music, current->user.social, current->user.dados_reservar);
        current = current->next;
    }
}

// Function to destroy user list
void destroy_user_list() {
    users_list *current = mem;
    while (current != NULL) {
        users_list *next = current->next; // Guarda o próximo nó
        free(current); // Libera a memória do nó atual
        current = next; // Move para o próximo nó
    }
    mem = NULL; // Garante que a memória compartilhada não aponte para um local inválido
}

// ============= AUX FUNCTIONS =============

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

// Creat Named pipes
void create_named_pipe(char *name){ 
  unlink(name);
  if ((mkfifo(name, O_CREAT|O_EXCL|0600)<0) && (errno != EEXIST)){
    printf("CANNOT CREATE NAMED PIPE -> EXITING\n");
    exit(1);
  }
}

// ============= PROCESSES AND THREADS =============

// Monitor engine process function
void monitor_engine(){
}

// Receiver funcion
void *receiver(void *arg){
    write_log("THREAD RECEIVER CREATED");

    int fd_user_pipe;
    char *buffer;

    if ((fd_user_pipe = open(USER_PIPE, O_RDONLY )) < 0) {
        perror("CANNOT OPEN USER_PIPE FOR READING\nS");
        exit(1);
    }

    while (1){
        buffer = read_from_pipe(fd_user_pipe);
        printf("%s\n", buffer);
    }


    pthread_exit(NULL);
}

// Sender function
void *sender(void *arg){
    write_log("THREAD SENDER CREATED");
    pthread_exit(NULL);
} 

// Authorization request manager function
void authorization_request_manager(){

    // Create named pipes
    create_named_pipe(USER_PIPE);
    create_named_pipe(BACK_PIPE);

    // Argumento para já será 0
    if (pthread_create(&receiver_thread, NULL, receiver, 0) != 0) {
        printf("CANNOT CREATE RECEIVER_THREAD\n");
        exit(1);
    }

    // Argumento para já será 1
    if (pthread_create(&sender_thread, NULL, sender,(void*)1) != 0) {
        printf("CANNOT CREATE SENDER_THREAD\n");
        exit(1);
    }

    // Closing threads
    if (pthread_join(receiver_thread, NULL) != 0){
        printf("CANNOT JOIN RECEIVER THREAD");
        exit(1);
    }

    if (pthread_join(sender_thread, NULL) != 0){
        printf("CANNOT JOIN SENDER THREAD");
        exit(1);
    }

}

// ============= MAIN PROGRAM =============

// Closing function
void cleanup(){

    int status, status1;

    write_log("5G_AUTH_PLATFORM SIMULATOR WAITING FOR LAST TASKS TO FINISH");

    // Wait for Authorization and Monitor engine
	if (waitpid(authorization_request_manager_process, &status, 0) == -1 ) write_log("waitpid\n");
    if (waitpid(monitor_engine_process, &status1, 0) == -1) write_log("waitpid\n");

    write_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");

    // Close Message Queue
    if (msgctl(mq_id, IPC_RMID, NULL) == -1) write_log("ERROR CLOSING P MESSAGE QUEUE\n");
    if (msgctl(mq_o_id, IPC_RMID, NULL) == -1) write_log("ERROR CLOSING OTHER MESSAGE QUEUE\n");
    if (msgctl(mq_v_id, IPC_RMID, NULL) == -1) write_log("ERROR CLOSING VIDEO MESSAGE QUEUE\n");

    // Close log file, destroy semaphoro and pipes
    if (sem_close(log_semaphore) == -1) write_log("ERROR CLOSING LOG SEMAPHORE\n");
    if (sem_unlink(LOG_SEM_NAME) == -1 ) write_log ("ERROR UNLINKING LOG SEMAPHORE\n");
    if (fclose(log_file) == EOF) write_log("ERROR CLOSIGN LOG FILE\n");
    if (unlink(USER_PIPE) == -1)write_log("ERROR UNLINKING PIPE USER_PIPE\n");
    if (unlink(BACK_PIPE) == -1)write_log("ERROR UNLINKING PIPE BACK_PIPE\n");
    //Delete shared memory
	if(shmdt(mem)== -1) write_log("ERROR IN shmdt");
	if(shmctl(shm_id,IPC_RMID, NULL) == -1) write_log("ERROR IN shmctl");

    //Free config malloc
    free(config);

    exit(0);
}

// Function to initialize log file and log semaphore
void init_log(){

    //Delete semaphore if is open
    sem_unlink(LOG_SEM_NAME);
    log_file = fopen("log.txt", "w");
    log_semaphore = sem_open(LOG_SEM_NAME, O_CREAT | O_EXCL, 0777, 1);
    if (log_semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    } 
}

// Function to initilize everything (process, semaphore...)
void init_program(){

    write_log("5G_AUTH_PLATFORM SIMULATOR STARTING");

    // Creating Message Queue
    if ((mq_id = msgget(IPC_PRIVATE, 0777|IPC_CREAT)) == -1){
        printf("ERRO CREATING MESSAGE QUEUE\n");
        exit(1);
    } 

    if ((mq_v_id = msgget(IPC_PRIVATE, 0777|IPC_CREAT)) == -1){
        printf("CANNOT CREAT VIDEO MESSAGE QUEUE\n");
        exit(1);
    } 

    if ((mq_o_id = msgget(IPC_PRIVATE, 0777|IPC_CREAT)) == -1){
        printf("CANNOT CREAT OTHER MESSAGE QUEUE\n");
        exit(1);
    } 

    //Create de shared memory
	int shsize= (sizeof(users_list)*config->max_mobile_users); // SM size
	shm_id = shmget(IPC_PRIVATE, shsize, IPC_CREAT | IPC_EXCL | 0777);
	if(shm_id==-1){
		printf("Erro no shmget\n");
		exit(1);
	}

	if((mem = (users_list *) shmat(shm_id, NULL,0)) == (users_list *) - 1){
		printf("Erro shmat\n");
		exit(0);
	}

    // Create monitor_engine_process
    monitor_engine_process = fork();
    if (monitor_engine_process == 0){

        // Writing for log file
        write_log("PROCESS MONITOR ENGINE CREATED");
        monitor_engine();
        exit(0);
    } else if (monitor_engine_process == -1){
        perror("CANNOT CREAT MONITOR ENGINE PROCESS\n");
        exit(1);
    }

    // Create authorization requeste manager process
    authorization_request_manager_process = fork();
    if (authorization_request_manager_process == 0){

        // Writing for log file
        write_log("PROCESS AUTHORIZATION REQUEST MANAGER CREATED");
        authorization_request_manager();
        exit(0);

    } else if (authorization_request_manager_process == -1){
        perror("CANNOT CREAT AUTHORIZATION REQUESTE MANAGER PROCESS\n");
        exit(1);
    }
}

int main(int argc, char* argv[]){

    if (argc != 2) {
        printf("5g_auth_platform {config-file}\n");
        return 0;
    }

    // Verify "config" file
    if (file_verification(argv[1]) != 0){
        return 0;
    }

    // Initialize log file and log sempahore
    init_log();

    // Initialize program
    init_program();

    // Signal to end program
    signal(SIGINT, cleanup);

    while(1){
        
    }
}