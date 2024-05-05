//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "funcoes.h"
#define DEBUG
#define LIST

// Mutex
pthread_mutex_t video_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t other_mutex = PTHREAD_MUTEX_INITIALIZER;


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
int check_authorization_free(shared_m *shdared_data, int i){
    sem_wait(user_sem);
    if (shdared_data->authorization_free[i] == 0){
        shdared_data->authorization_free[i] = 1;
        sem_post(user_sem);
        return 1;
    } else {
        sem_post(user_sem);
        return 0;
    }
}

// ============= Queue Functions =============

// Function to create a new node
struct Node* createNode(char *command) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->command = command;
    newNode->next = NULL;
    return newNode;
}

// Function to create a new queue
struct Queue* createQueue() {
    struct Queue* newQueue = (struct Queue*)malloc(sizeof(struct Queue));
    newQueue->front = newQueue->rear = NULL;
    return newQueue;
}

// Function to add an element to the rear of the queue
void enqueue(struct Queue* queue, char *command, int i) {

    pthread_mutex_t* mutex;

    // Determinar qual mutex deve ser usado com base no valor de 'i'
    if (i == 0) {
        mutex = &video_mutex;
    } else {
        mutex = &other_mutex;
    }

    char *command_copy = strdup(command); 
    struct Node* newNode = createNode(command_copy);

    pthread_mutex_lock(mutex);

    if (queue->front == NULL) {
        queue->front = queue->rear = newNode;
        pthread_mutex_unlock(mutex);
        return;
    }
    queue->rear->next = newNode;
    queue->rear = newNode;

    pthread_mutex_unlock(mutex);

}

// Function to take the last element
char* dequeue(struct Queue* queue, int i) {

    pthread_mutex_t* mutex;

    // Determinar qual mutex deve ser usado com base no valor de 'i'
    if (i == 0) {
        mutex = &video_mutex;
    } else {
        mutex = &other_mutex;
    }

    pthread_mutex_lock(mutex);

    if (queue->front == NULL) {
        pthread_mutex_unlock(mutex);        
        return NULL;
    }

    struct Node* temp = queue->front;
    char* command = (char*) malloc(strlen(temp->command) + 1);
    strcpy(command, temp->command);
    queue->front = temp->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp->command);
    free(temp);

    pthread_mutex_unlock(mutex);
    return command;
}

// Returns queue size
int queue_size(struct Queue* queue) {
    int count = 0;
    struct Node* current = queue->front;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Verify if queue is empty
int isEmpty(struct Queue* queue, int i) {
    pthread_mutex_t* mutex;

    if (i == 0) {
        mutex = &video_mutex;
    } else {
        mutex = &other_mutex;
    }

    pthread_mutex_lock(mutex);
    int empty = (queue->front == NULL);
    pthread_mutex_unlock(mutex);

    return empty;
}

// Print the Queue
void printQueue(struct Queue* queue, int i) {
    if (isEmpty(queue, i)) {
        printf("Queue is empty.\n");
        return;
    }
    struct Node* current = queue->front;
    printf("Queue contents:\n");
    while (current != NULL) {
        printf("%s\n", current->command);
        current = current->next;
    }
}

void write_Queue(struct Queue* queue) {
    char buf[124];
    if (queue->front == NULL) {
        printf("No tasks waiting to execute in internal queue\n");
        return;
    }
    struct Node* current = queue->front;
    printf("Queue contents:\n");
    while (current != NULL) {
        memset(buf, 0, 124); 
        strcpy(buf,current->command);
        printf("%s\n",buf);
        current = current->next;
    }
}

void destroyQueue(struct Queue* queue) {
    if ((queue->front == NULL) || (queue->rear == NULL)){
        free(queue);
        return;
    } else {
        struct Node* current = queue->front;
        struct Node* next = current->next;
        while (current != NULL) {
            next = current->next;
            free(current->command);
            free(current);
            current = next;
        }

        queue->front = NULL;
        queue->rear = NULL;

        free(queue);
    }
}

// ============= Linked List Functions =============

// Função para adicionar um usuário à lista ligada, recebendo diretamente uma struct user
void add_user_to_list(user new_user) {
    // Criando um novo nó para a lista
    sem_wait(user_sem);
    users_list *new_node = (users_list *)malloc(sizeof(users_list));
    if (new_node == NULL) {
        fprintf(stderr, "Failed to allocate memory for new user node\n");
        return;
    }

    // Inicializando o novo nó com os dados fornecidos
    new_node->user = new_user;
    new_node->next = NULL;

    // Adicionando o novo nó à lista ligada
    if (shm->mem == NULL) {
        // A lista está vazia, o novo nó se torna a cabeça da lista
        shm->mem = new_node;
    } else {
        // Encontrar o final da lista e adicionar o novo nó
        users_list *current = shm->mem;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    sem_post(user_sem);
}

int user_in_list(users_list *list, int id) {
    sem_wait(user_sem);
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            sem_post(user_sem);
            return 1;  // ID encontrado na lista
        }
        current = current->next;
    }
    sem_post(user_sem);
    return 0;  // ID não encontrado na lista
}

int is_dados_reservar_zero(users_list *list, int id) {
    sem_wait(user_sem);
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            if (current->user.dados_reservar == -1){
                sem_post(user_sem);
                return 1;
            } else {
                sem_post(user_sem);
                return 0;
            }

        }
        current = current->next;
    }
    sem_post(user_sem);
    return 0;  // Retorna 0 se o ID não for encontrado
}

void add_to_dados_reservar(users_list *list, int id, int add_value) {
    sem_wait(user_sem);
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            current->user.dados_reservar = add_value;  // Adiciona o valor a "dados_reservar"
            sem_post(user_sem);
            return;  // Retorna após adicionar o valor
        }
        current = current->next;
    }
}

int update_plafond(shared_m *shared_data, int user_id) {
    sem_wait(user_sem);
    users_list *current = shared_data->mem;  // Ponteiro para a cabeça da lista
    while (current != NULL) {
        if (current->user.id == user_id) {  // Verifica se é o usuário correto
            if (current->user.current_plafond >= current->user.dados_reservar) {
                current->user.current_plafond -= current->user.dados_reservar;  // Subtrai dados_reservar
                sem_post(user_sem);
                return 0;
            } else {
                current->user.current_plafond = 0;
                sem_post(user_sem);
                return 1;
            }
        }
        current = current->next;  // Move para o próximo elemento na lista
    }
    sem_post(user_sem);
    return -1;
}

// Print user list
void print_user_list() {
    sem_wait(user_sem);
    users_list *current = shm->mem;
    current = current->next;
    printf("Users in the list:\n");
    while (current != NULL) {
        printf("ID: %d, Plafond: %d, Actual Plafond %d, Reserved Data: %d\n",
               current->user.id, current->user.initial_plafond, current->user.current_plafond, current->user.dados_reservar);
                current = current->next;
    }

    printf("\n");
    sem_post(user_sem);
}

// ============= PROCESSES AND THREADS =============

// Monitor engine process function
void monitor_engine(){

    /*
    while (running){

        sem_wait(user_sem);
        users_list *current = shm->mem;
        sem_post(user_sem);

        current = current->next;
        while(current != NULL) {
            char log[124];
            queue_msg msg_queue;
            msg_queue.priority = 1;

            if ((current->user.initial_plafond * 0.2 == current->user.current_plafond) && 
                (current->user.initial_plafond * 0.1 < current->user.current_plafond) ){

                sprintf(log, "ALERT 80%% (USER ID = %d) TRIGGERED", current->user.id);
                msg_queue.id = current->user.id;
                strcpy(msg_queue.message, log);

                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);

                write_log(log);

            } else if ((current->user.initial_plafond * 0.1 == current->user.current_plafond) && 
                    (current->user.current_plafond > 0)) {

                sprintf(log, "ALERT 90%% (USER ID = %d) TRIGGERED", current->user.id);
                msg_queue.id = current->user.id;
                strcpy(msg_queue.message, log);

                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);

                write_log(log);
                
            } else if (current->user.current_plafond == 0) {

                sprintf(log, "ALERT 100%% (USER ID = %d) TRIGGERED", current->user.id);
                msg_queue.id = current->user.id;
                strcpy(msg_queue.message, log);

                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);

                write_log(log);
            }

            memset(log, 0, sizeof(log));
        }


    }
*/
}

// Receiver funcion
void *receiver(void *arg){
    write_log("THREAD RECEIVER CREATED");

    struct ThreadArgs *args = (struct ThreadArgs*) arg;
    struct Queue* queue_video = args->queue_video;
    struct Queue* queue_other = args->queue_other;

    int fd_user_pipe, fd_back_pipe;
    fd_set read_set;

    // Named pipes for reading
    if ((fd_user_pipe = open(USER_PIPE, O_RDONLY )) < 0) {
        perror("CANNOT OPEN USER_PIPE FOR READING\nS");
        exit(1);
    }

    /*
    if ((fd_back_pipe = open(BACK_PIPE, O_RDONLY)) < 0) {
        perror("CANNOT OPEN BACK_PIPE FOR READING\nS");
        exit(1);
    }
    
    
    int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
    max_fd += 1;  // Adiciona 1 ao maior descritor de arquivo
    */
    while (running) {

        // Open the FD
        FD_ZERO(&read_set);
        FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if (select(fd_user_pipe + 1, &read_set, NULL, NULL, NULL) > 0){
            if (FD_ISSET(fd_user_pipe, &read_set)){

                char *user_buffer;
                user_buffer = read_from_pipe(fd_user_pipe); 
 
                #ifdef DEBUG
                printf("USER_BUFFER: %s\n", user_buffer);
                #endif

                // Copy the original string
                char buffer[1024];                              
                strncpy(buffer, user_buffer, sizeof(buffer));   
                buffer[sizeof(buffer) - 1] = '\0';              

                char *token = strtok(buffer, "#");   //Token to get the values

                token = strtok(NULL, "#"); 

                if ((strcmp(token, "MUSIC") == 0) || (strcmp(token, "SOCIAL") == 0) || (strcmp(token, "VIDEO") == 0)){
                    if (strcmp(token, "VIDEO") == 0) { // GO TO VIDEO QUEUE

                        enqueue(queue_video, user_buffer, 0);
                        #ifdef DEBUG
                        printf("VIDEO USER BUFFER %s\n", user_buffer); 
                        #endif

                    } else { // GO TO OTHER QUEUE

                        enqueue(queue_other, user_buffer, 1);
                        #ifdef DEBUG
                        printf("OTHER USER BUFFER %s\n", user_buffer); 
                        #endif

                    }

                } else { // GO TO OTHER QUEUE

                    enqueue(queue_other, user_buffer, 1);

                    #ifdef DEBUG
                    printf("FIRST USER BUFFER %s\n", user_buffer); 
                    #endif

                }

                #ifdef DEBUG
                printf("VIDEO QUEUE SIZE %d\n", queue_size(queue_video));
                printf("OTHER QUEUE SIZE %d\n", queue_size(queue_other));
                #endif
                memset(buffer, 0 ,sizeof(buffer));
                FD_CLR(fd_user_pipe, &read_set); 

            } 
            /*if (FD_ISSET(fd_back_pipe, &read_set)){

                char *back_buffer;
                back_buffer = read_from_pipe(fd_back_pipe);

                #ifdef DEBUG
                printf("OTHER USER BUFFER %s\n", back_buffer); 
                #endif

                enqueue(queue_other, back_buffer, 1);


                FD_CLR(fd_back_pipe, &read_set);
            }
            */
        }
    }

    pthread_exit(NULL);
}

// Sender function
void *sender(void *arg){
    write_log("THREAD SENDER CREATED");

    struct ThreadArgs *args = (struct ThreadArgs*) arg;
    int (*pipes)[2] = args->pipes;
    struct Queue* queue_video = args->queue_video;
    struct Queue* queue_other = args->queue_other;

    for (int i = 0; i < config->auth_servers; i++){
        close(pipes[i][0]);
    }
    while (running){

        if (!isEmpty(queue_video, 0)){
            while(!isEmpty(queue_video, 0)){
                char *msg_video = dequeue(queue_video, 0);
                
                #ifdef DEBUG
                printf("SENDER VIDEO: %s\n", msg_video);
                #endif

                for (int i = 0; i < config->auth_servers; i++){

                    if (check_authorization_free(shm, i)){

                        char aux[64];
                        strcpy(aux, msg_video);

                        char log_msg[124];
                        sprintf(log_msg, "SENDER: VIDEO AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux, "#") , i + 1);

                        ssize_t bytes_written = write(pipes[i][1], msg_video, strlen(msg_video) + 1);
                        if (bytes_written < 0){
                            perror("ERROR WRITING");
                        } else {
                            write_log(log_msg);
                        }

                        break;
                    }   
                }  
            }
        } else {
            if (!isEmpty(queue_other, 1)){
                while(!isEmpty(queue_other, 1)){
                    char *msg_other = dequeue(queue_other, 1);
                    
                    #ifdef DEBUG
                    printf("SENDER OTHER: %s\n", msg_other);
                    #endif

                    for (int i = 0; i < config->auth_servers; i++){

                        if (check_authorization_free(shm, i)){

                            char aux[64];
                            strcpy(aux, msg_other);

                            char log_msg[1024];
                            sprintf(log_msg, "SENDER: OTHER AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux,"#"), i + 1);

                            ssize_t bytes_written = write(pipes[i][1], msg_other, strlen(msg_other) + 1);
                            if (bytes_written < 0){
                                perror("ERROR WRITING");
                            } else {
                                write_log(log_msg);
                            }

                            break;

                        }
                    }
                }  
            }
        }
    }

    pthread_exit(NULL);
} 

// Autorization engines
void authorization_engine(int id, int (*pipes)[2]){

    char log[64];
    sprintf(log, "AUTHORIZATION ENGINE %d INIT", id + 1);
    write_log(log);

    close(pipes[id][1]);
     
    while(running){

        time_t start, end;
        time(&start);
        
        char msg[1024];
        ssize_t bytes_read = read(pipes[id][0], msg, sizeof(msg));

        if (bytes_read > 0){

            #ifdef DEBUG
            printf("AUTHORIZATION ENGINE %d MSG: %s\n",id + 1 ,msg);
            #endif

            user aux;

            // Copy the original string
            char buffer[1024];                              
            strncpy(buffer, msg, sizeof(buffer));   
            buffer[sizeof(buffer) - 1] = '\0';              

            char *token = strtok(buffer, "#");   //Token to get the values

            if (atoi(token) == 1){
                printf("MENSAGEM BACKOFFICE %s\n", msg);

            } else {

                aux.id = atoi(token);

                if (user_in_list(shm->mem, aux.id) == 0){ // SE AINDA NÃO TIVER NA LISTA
                    token = strtok(NULL, "#");                
                    aux.initial_plafond = atoi(token);
                    aux.current_plafond = atoi(token);
                    aux.dados_reservar = -1;
                    add_user_to_list(aux);

                } else { //SE JA TIVER NA LISTA

                    if (is_dados_reservar_zero(shm->mem, aux.id) == 1){ // if aux.dados_reserver == -1
                        token = strtok(NULL, "#");
                        token = strtok(NULL, "#");

                        // adiciona os valores que faltava à struct do user para 
                        add_to_dados_reservar(shm->mem, aux.id, atoi(token));
                    }

                    int plafond = update_plafond(shm, aux.id);
                    if (plafond == 1){ // if plafond used
                        char log[124];
                        sprintf(log, "USER PLAFOND IS OVER (ID = %d) ", aux.id);
                        write_log(log);
                    } else if (plafond == -1){
                        write_log("USER DONT EXIST");
                        exit(1);
                    }
                }
            
                #ifdef LIST
                print_user_list();
                #endif 

                char log[124];
                sprintf(log, "AUTHORIZATION_ENGINE %d: VIDEO AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id + 1, aux.id);
                write_log(log);
                

            }

            time(&end);

            double elapsed = difftime(end, start);
            if (elapsed/1000 < config->auth_proc_time){
                sleep((config->auth_proc_time - elapsed) / 1000);
            }

            sem_wait(user_sem);
            shm->authorization_free[id] = 0;
            sem_post(user_sem);
        }


    }
}

// Authorization request manager function
void authorization_request_manager(){

    pid_t main_pid = getpid();

    // Create named pipes
    create_named_pipe(USER_PIPE);
    create_named_pipe(BACK_PIPE);

    // Create unnamed pipes
    int pipes[config->auth_servers][2];
    create_unnamed_pipes(pipes);

    // Create authorization engine processes
    for (int i = 0; i < config->auth_servers; i++){
        if (main_pid == getpid()){
            if (fork() == 0){
                authorization_engine(i, &pipes[i]);
                exit(0);
            }
        }
    }

    // Create Queues
    queue_video = createQueue();
    queue_other = createQueue();

    struct ThreadArgs thread_args = { .pipes = pipes, .queue_video = queue_video, .queue_other = queue_other };

    // Create Threads
    if (pthread_create(&receiver_thread, NULL, receiver, (void *) &thread_args) != 0) {
        printf("CANNOT CREATE RECEIVER_THREAD\n");
        exit(1);
    }

    if (pthread_create(&sender_thread, NULL, sender, (void*) &thread_args) != 0) {
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

    destroyQueue(queue_other);
    destroyQueue(queue_video);

}

// ============= MAIN PROGRAM =============

// Closing function
void cleanup(){

    int status, status1;
    running = 0;

    write_log("5G_AUTH_PLATFORM SIMULATOR WAITING FOR LAST TASKS TO FINISH");

    // Wait for Authorization and Monitor engine
	if (waitpid(authorization_request_manager_process, &status, 0) == -1 ) write_log("waitpid\n");
    if (waitpid(monitor_engine_process, &status1, 0) == -1) write_log("waitpid\n");

    write_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");

    // Close Message Queue
    if (msgctl(mq_id, IPC_RMID, NULL) == -1) write_log("ERROR CLOSING P MESSAGE QUEUE\n");

    pthread_mutex_destroy(&video_mutex);
    pthread_mutex_destroy(&other_mutex);
   
    // Close log file, destroy semaphoro and pipes
    if (sem_close(log_semaphore) == -1) write_log("ERROR CLOSING LOG SEMAPHORE\n");
    if (sem_unlink(LOG_SEM_NAME) == -1 ) write_log ("ERROR UNLINKING LOG SEMAPHORE\n");
    if (sem_close(user_sem) == -1) write_log("ERROR CLOSING USER SEMAPHORE\n");
    if (sem_unlink(USER_SEM) == -1 ) write_log ("ERROR UNLINKING USER SEMAPHORE\n");
    if (sem_close(autho_free) == -1) write_log("ERROR CLOSING AUTHORIZATION ENGINE SEMAPHORE\n");
    if (sem_unlink(AUTH_SEM) == -1 ) write_log ("ERROR UNLINKING AUTHORIZATION ENGINE SEMAPHORE\n");
    if (fclose(log_file) == EOF) write_log("ERROR CLOSIGN LOG FILE\n");
    if (unlink(USER_PIPE) == -1)write_log("ERROR UNLINKING PIPE USER_PIPE\n");
    if (unlink(BACK_PIPE) == -1)write_log("ERROR UNLINKING PIPE BACK_PIPE\n");

    //Delete shared memory
	if(shmdt(shm)== -1) write_log("ERROR IN shmdt");
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

    running = 1;

    // Creating Message Queue
    if ((mq_id = msgget(IPC_PRIVATE, 0777|IPC_CREAT)) == -1){
        printf("ERRO CREATING MESSAGE QUEUE\n");
        exit(1);
    } 

    //Create de shared memory
	int shsize= (sizeof(shared_m) + sizeof(users_list)*config->max_mobile_users + sizeof(int) * config->auth_servers + 1); // SM size
	shm_id = shmget(IPC_PRIVATE, shsize, IPC_CREAT | IPC_EXCL | 0777);
	if(shm_id==-1){
		printf("Erro no shmget\n");
		exit(1);
	}

	if((shm = (shared_m *) shmat(shm_id, NULL,0)) == (shared_m *) - 1){
		printf("Erro shmat\n");
		exit(0);
	}

    shm->mem = (users_list *)(shm + 1);
    shm->authorization_free = (int *)(shm->mem + 1);

    for (int i = 0; i < config->auth_servers + 1; i++){
        shm->authorization_free[i] = 0;
    }

    // Init Semaphore
    sem_unlink(USER_SEM);
    user_sem = sem_open(USER_SEM, O_CREAT | O_EXCL, 0777, 1);
    if (user_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_unlink(AUTH_SEM);
    autho_free = sem_open(AUTH_SEM, O_CREAT | O_EXCL, 0777, 1);
    if (autho_free == SEM_FAILED) {
        perror("sem_open");
        exit(1);
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

// Main function
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

    while(running){}
}