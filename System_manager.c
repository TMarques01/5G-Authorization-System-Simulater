//Mariana Sousa 2022215999
//Tiago Marques 2022210638

// read from pipe
// mallocs para a lista ligad

#include "System_manager.h"
#include "funcoes.h"
//#define DEBUG
#define LIST
#define TAM 1024

// Mutex
pthread_mutex_t video_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t other_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sender_cond = PTHREAD_COND_INITIALIZER;

pthread_mutexattr_t monitor_attrmutex;
pthread_condattr_t monitor_attrcond;
int monitor_variable = 0;


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

    sem_wait(user_sem);
    if (shm->authorization_free[i] == 0){
        shm->authorization_free[i] = 1;
        #ifdef DEBUG
        //for (int i = 0; i < config->auth_servers; i++) printf("AUTHORIZATION_FREE[%d]: %d \n", i + 1, shm->authorization_free[i]);
        #endif
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
int queue_size(struct Queue* queue, int i) {

    /*
    pthread_mutex_t* mutex;

    if (i == 0) {
        mutex = &video_mutex;
    } else {
        mutex = &other_mutex;
    }
    */
    //pthread_mutex_lock(mutex);
    int count = 0;
    struct Node* current = queue->front;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    //pthread_mutex_unlock(mutex);
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

int update_plafond(int id) {
    sem_wait(user_sem);

    for (int i = 0; i < config->max_mobile_users; i++){
        if (shm->mem[i].id == id){
            if (shm->mem[i].current_plafond >= shm->mem[i].dados_reservar){
                shm->mem[i].current_plafond -= shm->mem->dados_reservar;
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

// ============= PROCESSES AND THREADS =============

// Monitor engine process function
void monitor_engine(){

    while (running){

        //pthread_mutex_lock(&shm->monitor_mutex);
        while(monitor_variable == 0){

            //pthread_cond_wait(&shm->monitor_cond, &shm->monitor_mutex);
        }


        sem_wait(user_sem);
        for (int i = 0; i < config->max_mobile_users; i++){
            if (shm->mem[i].id == -999) continue;
            char log[124];
            //printf("id a confirmar %d\n", shm->mem[i].id);
            queue_msg msg_queue;

            if (shm->mem[i].current_plafond == 0) {
                sprintf(log, "ALERT 100%% (USER ID = %d) TRIGGERED", shm->mem[i].id);

                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);
                
                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);
                continue;

            } else if ((shm->mem[i].initial_plafond * 0.1 == shm->mem[i].current_plafond)) 
            //|| 
            //        ((shm->mem[i].current_plafond > 0) && (shm->mem[i].current_plafond < shm->mem[i].initial_plafond * 0.1))) 
            {

                sprintf(log, "ALERT 90%% (USER ID = %d) TRIGGERED", shm->mem[i].id);
                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);

                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);
                continue;
               
            } else if ((shm->mem[i].initial_plafond * 0.2 ==shm->mem[i].current_plafond))
                //((shm->mem[i].initial_plafond * 0.1 > shm->mem[i].current_plafond) &&
                //(shm->mem[i].initial_plafond * 0.2 < shm->mem[i].current_plafond)) )
                {

                sprintf(log, "ALERT 80%% (USER ID = %d) TRIGGERED", shm->mem[i].id);
                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);

                msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0);
                continue;

            }
            memset(log, 0, sizeof(log));
        }

        monitor_variable = 0;
        sem_post(user_sem);
        //pthread_mutex_unlock(&shm->monitor_mutex);
    }
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
    if ((fd_user_pipe = open(USER_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
        perror("CANNOT OPEN USER_PIPE FOR READING\nS");
        exit(1);
    }

    
    if ((fd_back_pipe = open(BACK_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
        perror("CANNOT OPEN BACK_PIPE FOR READING\nS");
        exit(1);
    }
    
    
    int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
    max_fd += 1;  // Adiciona 1 ao maior descritor de arquivo
    
    while (running) {

        // Open the FD
        FD_ZERO(&read_set);
        FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if (select(max_fd, &read_set, NULL, NULL, NULL) > 0){
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
                        pthread_cond_signal(&sender_cond);
                        #ifdef DEBUG
                        printf("VIDEO USER BUFFER %s\n", user_buffer); 
                        #endif

                    } else { // GO TO OTHER QUEUE

                        enqueue(queue_other, user_buffer, 1);
                        pthread_cond_signal(&sender_cond);
                        #ifdef DEBUG
                        printf("OTHER USER BUFFER %s\n", user_buffer); 
                        #endif

                    }

                } else { // GO TO OTHER QUEUE

                    enqueue(queue_other, user_buffer, 1);
                    pthread_cond_signal(&sender_cond);

                    #ifdef DEBUG
                    printf("FIRST USER BUFFER %s\n", user_buffer); 
                    #endif

                }

                #ifdef DEBUG
                printf("VIDEO QUEUE SIZE %d\n", queue_size(queue_video, 0));
                printf("OTHER QUEUE SIZE %d\n", queue_size(queue_other, 1));
                #endif
                memset(buffer, 0 ,sizeof(buffer));
                FD_CLR(fd_user_pipe, &read_set); 

            } 
            if (FD_ISSET(fd_back_pipe, &read_set)){

                char *back_buffer;
                back_buffer = read_from_pipe(fd_back_pipe);

                #ifdef DEBUG
                printf("OTHER USER BUFFER %s\n", back_buffer); 
                #endif

                enqueue(queue_other, back_buffer, 1);
                pthread_cond_signal(&sender_cond);



                FD_CLR(fd_back_pipe, &read_set);
            }
            
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

        pthread_mutex_lock(&video_mutex);
        while (queue_size(queue_video, 0) == 0 && (queue_size(queue_other, 1) == 0)){
            pthread_cond_wait(&sender_cond, &video_mutex);
        }
        pthread_mutex_unlock(&video_mutex);

        if (!(isEmpty(queue_video, 0))){
            while(!isEmpty(queue_video, 0)){
                char *msg_video = dequeue(queue_video, 0);

                #ifdef DEBUG
                printf("SENDER VIDEO: %s\n", msg_video);
                #endif

                for (int i = 0; i < config->auth_servers; i++){

                    if (check_authorization_free(i)){

                        char aux[TAM];
                        strcpy(aux, msg_video);

                        char log_msg[TAM];
                        sprintf(log_msg, "SENDER: VIDEO AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux, "#") , i + 1);
                        ssize_t bytes_written = write(pipes[i][1], msg_video, strlen(msg_video) + 1);
                        if (bytes_written < 0){
                            perror("ERROR WRITING");
                        } else {
                            write_log(log_msg);
                            break;
                        }
                    } 
                }  
            }
        } else {
            if (!isEmpty(queue_other, 1)){   
                char *msg_other = dequeue(queue_other, 1);
                
                #ifdef DEBUG
                printf("SENDER OTHER: %s\n", msg_other);
                #endif

                for (int i = 0; i < config->auth_servers; i++){

                    if (check_authorization_free(i)){
                        char aux[TAM];
                        strcpy(aux, msg_other);

                        char log_msg[TAM];
                        sprintf(log_msg, "SENDER: OTHER AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux,"#"), i + 1);

                        ssize_t bytes_written = write(pipes[i][1], msg_other, strlen(msg_other) + 1);
                        if (bytes_written < 0){
                            perror("ERROR WRITING");
                        } else {
                            write_log(log_msg);
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

        char msg[1024];
        ssize_t bytes_read = read(pipes[id][0], msg, sizeof(msg));

        if (bytes_read > 0){
            time(&start);
            
            printf("AUTHORIZATION ENGINE %d MSG: %s\n",id + 1 ,msg);

            user aux;

            // Copy the original string
            char buffer[TAM];                              
            strncpy(buffer, msg, sizeof(buffer));   
            buffer[sizeof(buffer) - 1] = '\0';              

            char *token = strtok(buffer, "#");   //Token to get the values

            if (atoi(token) == 1){
                printf("MENSAGEM BACKOFFICE %s\n", msg);
                if (strcmp(msg, "1#data_stats") == 0){

                    sem_wait(user_sem);

                    char tam[TAM];
                    sprintf(tam,"SERVIÇO   TOTAL DATA  AUTH REQS\nVIDEO       %d          %d\nMUSIC       %d          %d\nSOCIAL      %d          %d\n", 
                    shm->stats.td_video, shm->stats.ar_video, shm->stats.td_music, shm->stats.ar_music, shm->stats.td_social, shm->stats.ar_social);
                
                    sem_post(user_sem);

                    queue_msg back_msg;
                    back_msg.id = 1;
                    back_msg.priority = 0;
                    strcpy(back_msg.message,tam);

                    //msgsnd(mq_id, &back_msg, sizeof(back_msg) - sizeof(long), 0);
                    printf("sended\n");
                } else {    

                }

            } else {

                aux.id = atoi(token);

                if (user_in_list(aux.id) == 0){ // SE AINDA NÃO TIVER NA LISTA
                    token = strtok(NULL, "#");                
                    int valor = atoi(token);
                    if (valor != 0){
                        aux.initial_plafond = valor;
                        aux.current_plafond = valor;
                        aux.dados_reservar = -1;

                        add_user_to_list(aux);
                    }
                } else if (user_in_list(aux.id) == 1) { //SE JA TIVER NA LISTA
                    token = strtok(NULL, "#");
                    char type[TAM];
                    strcpy(type, token);
                    if (is_dados_reservar_zero(aux.id) == 1){ // if aux.dados_reserver == -1

                        if ((strcmp(token, "VIDEO") == 0) || (strcmp(token, "SOCIAL") == 0) || (strcmp(token, "MUSIC") == 0)){
                            token = strtok(NULL, "#");
                            // adiciona os valores que faltava à struct do user para 
                            add_to_dados_reservar(aux.id, atoi(token));

                            // cond signal
                        }
                    }



                    int plafond = update_plafond(aux.id);

                    //pthread_mutex_lock(&shm->monitor_mutex);
                    //monitor_variable = 1;
                    //pthread_mutex_unlock(&shm->monitor_mutex);
                    
                    //pthread_cond_signal(&shm->monitor_cond);

                    if (plafond == 1){ // if all plafond is used
                        char log[124];
                        sprintf(log, "USER PLAFOND IS OVER (ID = %d) ", aux.id);
                        write_log(log);
                    } else if (plafond == -1){
                        write_log("USER DONT EXIST");
                        exit(1);
                    }

                }
            
                char log[124];
                sprintf(log, "AUTHORIZATION_ENGINE %d: VIDEO AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED", id + 1, aux.id);
                write_log(log);
            
            }

            #ifdef LIST
            print_user_list();
            #endif 

            time(&end);

            double elapsed = difftime(end, start);
            if (elapsed/1000 < config->auth_proc_time){
                sleep((config->auth_proc_time - elapsed) / 1000);
            }

            sem_wait(user_sem);
            shm->authorization_free[id] = 0;
            sem_post(user_sem);

        }

        memset(msg, 0, sizeof(msg));
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

    for (int i = 0; i < config->auth_servers; i++) wait(NULL);

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
/*
    pthread_mutex_destroy(&video_mutex);
    pthread_mutex_destroy(&other_mutex);
    pthread_cond_destroy(&sender_cond);

    pthread_mutex_destroy(&shm->monitor_mutex);
    pthread_mutexattr_destroy(&monitor_attrmutex);
    pthread_cond_destroy(&shm->monitor_cond);
    pthread_condattr_destroy(&monitor_attrcond);
*/
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
	if(shmctl(shm_id, IPC_RMID, NULL) == -1) write_log("ERROR IN shmctl");

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
	int shsize= (sizeof(shared_m)  + sizeof(user)*config->max_mobile_users + sizeof(int) * (config->auth_servers + 1)); // SM size
	shm_id = shmget(IPC_PRIVATE, shsize, IPC_CREAT | IPC_EXCL | 0777);
	if(shm_id==-1){
		printf("Erro no shmget\n");
		exit(1);
	}

	if((shm = (shared_m *) shmat(shm_id, NULL,0)) == (shared_m *) - 1){
		printf("Erro shmat\n");
		exit(0);
	}

    shm->mem = (user *)(shm + 1);
    shm->authorization_free = (int *)(shm->mem + config->max_mobile_users);

    for (int i = 0; i < config->max_mobile_users; i++){
        shm->mem[i] = (user){.initial_plafond = -999,
                            .current_plafond = -999,
                            .dados_reservar = -999,
                            .id = -999,
                            .index = -999}; 
    }

    for (int i = 0; i < config->auth_servers + 1; i++){
        shm->authorization_free[i] = 0;
    }

    shm->stats.ar_music = 0;
    shm->stats.ar_social = 0;
    shm->stats.ar_video = 0;
    shm->stats.td_music = 0;
    shm->stats.td_video = 0;
    shm->stats.td_social = 0;

/*
    pthread_mutexattr_init(&monitor_attrmutex);
    pthread_mutexattr_setpshared(&monitor_attrmutex, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&monitor_attrcond);
    pthread_condattr_setpshared(&monitor_attrcond, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm->monitor_mutex, &monitor_attrmutex);
    pthread_cond_init(&shm->monitor_cond, &monitor_attrcond);
*/

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

    sem_unlink(FIFO_FULL);
    fifo_full = sem_open(FIFO_FULL, O_CREAT | O_EXCL, 0777, 0);
    if (fifo_full == SEM_FAILED) {
        perror("sem_open");
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