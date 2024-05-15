//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "funcoes.h"

#define TAM 1024

// Mutex
pthread_mutex_t video_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t other_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sender_cond = PTHREAD_COND_INITIALIZER;

int init_unnamed_pipes(){
    pipes = malloc(config->auth_servers * sizeof(*pipes));
    for (int i = 0; i < config->auth_servers + 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Failed to create pipe");

            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return -1;
        }
    }
    return 0;
}

// ============= Queue Functions =============

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

    // wich mutex we will use (i = 0 -> video_mutex | i = 1 -> other_mutex)
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
    if (command == NULL) { // Check for malloc failure
        pthread_mutex_unlock(mutex);
        write_log("ERROR ALLOCATING MEMORY");
        return NULL;
    }

    strcpy(command, temp->command);
    queue->front = temp->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    /* else {

        time(&queue->front->end);
        double elapsed = difftime(queue->front->end, queue->front->start);
        pthread_mutex_unlock(mutex);
        if (!verify_queue_time(elapsed, command)) {
            free(temp->command);
            free(temp);

            return command;
        }
    }*/

    free(temp->command);
    free(temp);
    pthread_mutex_unlock(mutex);

    return command; // Adjust error handling or return an error code/message as needed
}

// Function to create a new node
struct Node* createNode(char *command) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->command = command;
    time(&newNode->start);
    newNode->next = NULL;
    return newNode;
}

// Function to create a new queue
struct Queue* createQueue() {
    struct Queue* newQueue = (struct Queue*)malloc(sizeof(struct Queue));
    newQueue->front = newQueue->rear = NULL;
    return newQueue;
}

// Returns queue size
int queue_size(struct Queue* queue, int i) {
    
    int count = 0;
    struct Node* current = queue->front;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

// Print last requestes in the queue
void write_Queue(struct Queue* queue) {
    char buf[124];
    if (queue->front == NULL) {
        write_log("NO TASKS WAITING TO EXECUTE IN INTERNAL QUEUE");
        return;
    }
    struct Node* current = queue->front;
    write_log("QUEUE CONTENTS:");
    while (current != NULL) {
        memset(buf, 0, 124); 
        strcpy(buf,current->command);
        write_log(buf);
        current = current->next;
    }
}

// destroy the queueu
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

// ============= PROCESSES AND THREADS =============

// Thread that send statistic to back office
void *back_stats(void *args){

    while (verify_running()){

        sleep(15);

        sem_wait(stats_sem);
        
        char tam[TAM];
        sprintf(tam,    "SERVIÇO   TOTAL DATA  AUTH REQS\nVIDEO       %d          %d\nMUSIC       %d          %d\nSOCIAL      %d          %d\n", 
        shm->stats.td_video, shm->stats.ar_video, shm->stats.td_music, shm->stats.ar_music, shm->stats.td_social, shm->stats.ar_social);

        queue_msg back_msg;
        long prty = 1;
        back_msg.priority = prty;
        strcpy(back_msg.message, tam);

        if (msgsnd(mq_id, &back_msg, sizeof(queue_msg) - sizeof(long), 0) == -1) write_log("ERROR WRITING FOR MESSAGE QUEUE");
        
        sem_post(stats_sem);

        sleep(15);
        
    }
    pthread_exit(NULL);
}

// Sende warning 
void *monitor_warnings(void *args){

    while (verify_running()){
        
        sem_wait(monitor_sem);

        sem_wait(user_sem);

        for (int i = 0; i < config->max_mobile_users; i++){

            if (shm->mem[i].id == -999) continue;

            char log[124];

            queue_msg msg_queue;

            if (shm->mem[i].current_plafond == 0 && shm->mem[i].triggers == 2) {
                sprintf(log, "ALERT 100%% (USER ID = %d) TRIGGERED", shm->mem[i].id);

                shm->mem[i].triggers++;

                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);
                
                msgsnd(mq_id, &msg_queue, sizeof(queue_msg) - sizeof(long), 0);

                continue;

            } else if ((shm->mem[i].initial_plafond * 0.1 >= shm->mem[i].current_plafond) &&
                        (shm->mem[i].triggers == 1)) {

                shm->mem[i].triggers++;

                sprintf(log, "ALERT 90%% (USER ID = %d) TRIGGERED", shm->mem[i].id);
                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);

				if (msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0) == -1) write_log("ERROR WRITING FOR MESSAGE QUEUE");
            
                continue;

            } else if ((shm->mem[i].initial_plafond * 0.2 >= shm->mem[i].current_plafond) &&
                        (shm->mem[i].triggers == 0)) {

                shm->mem[i].triggers++;

                sprintf(log, "ALERT 80%% (USER ID = %d) TRIGGERED", shm->mem[i].id);
                msg_queue.priority = shm->mem[i].id;
                strcpy(msg_queue.message, log);
                write_log(log);

				if (msgsnd(mq_id, &msg_queue, sizeof(msg_queue) - sizeof(long), 0) == -1) write_log("ERROR WRITING FOR MESSAGE QUEUE");
                
                continue;
            }

            memset(log, 0, sizeof(log));
        }

        sem_post(user_sem);
    }

    pthread_exit(NULL);
}

// Monitor engine process function
void monitor_engine(){

    signal(SIGINT, monitor_signal);

    if (pthread_create(&back_office_stats, NULL, back_stats, NULL ) != 0) {
        write_log("CANNOT CREATE BACK_OFFICE_STATS_THREAD");
        exit(1);
    }

    if (pthread_create(&monitor_warning, NULL, monitor_warnings, NULL ) != 0) {
        write_log("CANNOT CREATE BACK_OFFICE_STATS_THREAD");
        exit(1);
    }

    // Join threads
    if (pthread_join(back_office_stats, NULL) != 0){
        printf("CANNOT JOIN BACK OFFICE STATS THREAD");
        exit(1);
    }

    if (pthread_join(monitor_warning, NULL) != 0){
        printf("CANNOT JOIN MONITOR WARING STATS THREAD");
        exit(1);
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
    if ((fd_user_pipe = open(USER_PIPE, O_RDWR | O_NONBLOCK)) < 0) {
        perror("CANNOT OPEN USER_PIPE FOR READING\nS");
        exit(1);
    }
    
    if ((fd_back_pipe = open(BACK_PIPE, O_RDWR | O_NONBLOCK)) < 0) {
        perror("CANNOT OPEN BACK_PIPE FOR READING\nS");
        exit(1);
    }
    
    int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
    max_fd += 1;
    
    while (verify_running()) {

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
 
                // verify if the video queue is full
                if ((strcmp(token, "VIDEO") == 0) && (queue_size(queue_video, 0) >= config->queue_pos)){

                    char log[124];
                    sprintf(log, "VIDEO QUEUE IS FULL. DISCARDING %s", user_buffer);
                    write_log(log);

                // verify if the other queue is full 
                } else if (queue_size(queue_other, 1) >= config->queue_pos) {

                    char log[124];
                    sprintf(log, "OTHER QUEUE IS FULL. DISCARDING %s", user_buffer);
                    write_log(log);
                    
                } else {

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

                if (back_buffer != NULL){   

                    #ifdef DEBUG
                    printf("OTHER USER BUFFER %s\n", back_buffer); 
                    #endif

                    enqueue(queue_other, back_buffer, 1);
                    pthread_cond_signal(&sender_cond);

                }

                FD_CLR(fd_back_pipe, &read_set);
            }
        }
    }

    pthread_exit(NULL);
}

// Sender function
void *sender(void *arg){
    write_log("THREAD SENDER CREATED");

    int n_auth_servers = config->auth_servers;
    int flag = 0;

    struct ThreadArgs *args = (struct ThreadArgs*) arg;
    struct Queue* queue_video = args->queue_video;
    struct Queue* queue_other = args->queue_other;

    for (int i = 0; i < config->auth_servers; i++) close(pipes[i][0]);

    while (verify_running()) {
        
        // Waiting for the signal to advance
        pthread_mutex_lock(&video_mutex);
        while (queue_size(queue_video, 0) == 0 && (queue_size(queue_other, 1) == 0)){
            pthread_cond_wait(&sender_cond, &video_mutex);
        }
        pthread_mutex_unlock(&video_mutex);

        // Verify if the queue is full
        #ifdef EXTRA_AE
        if ((queue_size(queue_video, 0) == config->queue_pos || queue_size(queue_other, 1) == config->queue_pos) && flag == 0){

            if (queue_size(queue_video, 0) == config->queue_pos){
                flag = 2; // flag 2 if the video queue is full
            } else if (queue_size(queue_other, 1) == config->queue_pos){
                flag = 3; // flag = 3 if the other queue is full
            }

            n_auth_servers++;

            write_log("CREATING EXTRA AUTHORIZATION ENGINE");
        }

        // verify if the queue is 50% size and remove the extra AE
        if (n_auth_servers == config->auth_servers + 1){ 
            if ((queue_size(queue_video, 0) <= (config->queue_pos/2)) && flag == 2){

                close(pipes[config->auth_servers][1]);
                flag = 0;
                n_auth_servers--;
                write_log("KILLING EXTRA AUTHORIZATION ENGINE");

            } else if ((queue_size(queue_other, 1 ) <= (config->queue_pos/2)) && flag == 3){

                close(pipes[config->auth_servers][1]);                
                flag = 0;
                n_auth_servers--;
                write_log("KILLING EXTRA AUTHORIZATION ENGINE");
            }
        }
        #endif

        // Verify if the video queue is empty
        if (!(isEmpty(queue_video, 0))){
            while(!isEmpty(queue_video, 0)){
                for (int i = 0; i < n_auth_servers; i++){
                    if (check_authorization_free(i)){ // check wich AE is available

                        char *msg_video = dequeue(queue_video, 0);

                        #ifdef DEBUG
                        printf("SENDER VIDEO: %s\n", msg_video);
                        #endif

                        if (msg_video == NULL){ // if the request time overpassed the max time
                            write_log("DISCARDING A REQUEST");
                            sem_wait(autho_free);
                            shm->authorization_free[i] = 0;
                            sem_post(autho_free);
                            break;

                        } else {

                            char aux[TAM];
                            strcpy(aux, msg_video);

                            char log_msg[TAM];
                            sprintf(log_msg, "SENDER: VIDEO AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux, "#") , i + 1);
                            
                            sem_wait(user_sem);
                            ssize_t bytes_written = write(pipes[i][1], msg_video, strlen(msg_video) + 1);

                            if (bytes_written < 0){
                                write_log("ERROR WRITING TO THE UNNAMED PIPE");

                                sem_wait(autho_free);
                                shm->authorization_free[i] = 0;
                                sem_post(autho_free);
                                sem_post(user_sem);
                                break;                            
                                
                            } else {
                                
                                sem_wait(stats_sem);
                                shm->stats.ar_video++;
                                sem_post(stats_sem);

                                write_log(log_msg);
                                sem_post(user_sem);
                                break;
                            }
                        }
                    } 
                }  
            }
        } else { // if the video queue is not empty, will check the other queue
            if (!isEmpty(queue_other, 1)){   
                for (int i = 0; i < n_auth_servers; i++){
                    if (check_authorization_free(i)){ // check wich AE is available

                        char *msg_other = dequeue(queue_other, 1);

                        #ifdef DEBUG
                        printf("SENDER OTHER: %s\n", msg_other);
                        #endif

                        if (msg_other == NULL){ // if the request time overpassed the max time
                            write_log("DISCARDING A REQUEST");
                            sem_wait(autho_free);
                            shm->authorization_free[i] = 0;
                            sem_post(autho_free);
                            break;

                        }                       
                        
                        char aux[TAM];
                        strcpy(aux, msg_other);

                        char log_msg[TAM];
                        sprintf(log_msg, "SENDER: OTHER AUTHORIZATION REQUEST (ID = %s) SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d", strtok(aux,"#"), i + 1);
                        char *type = strtok(NULL, "#");

                        // sending to the unnamed pipe
                        sem_wait(user_sem); 
                        ssize_t bytes_written = write(pipes[i][1], msg_other, strlen(msg_other) + 1);


                        if (bytes_written < 0){

                            write_log("ERROR WRITING TO THE UNNAMED PIPE");

                            sem_wait(autho_free);
                            shm->authorization_free[i] = 0;
                            sem_post(autho_free);
                            sem_post(user_sem);
                            break;   

                        } else {

                            if (strcmp(type, "SOCIAL") == 0){ // add to the stats variables according to the type

                                sem_wait(stats_sem);
                                shm->stats.ar_social++;
                                sem_post(stats_sem);

                            } else if (strcmp(type, "MUSIC") == 0){

                                sem_wait(stats_sem);
                                shm->stats.ar_music++;
                                sem_post(stats_sem);                    
                            }

                            write_log(log_msg);
                            sem_post(user_sem);
                            break;
                        }
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
} 

// Authorization engines
void authorization_engine(int id){

    char log[64];
    sprintf(log, "AUTHORIZATION ENGINE %d READY", id + 1);
    write_log(log);

    close(pipes[id][1]);

    while(verify_running()){

        char msg[1024];

        ssize_t bytes_read = read(pipes[id][0], msg, sizeof(msg));

        if (bytes_read > 0){

            time_t start, end;
            time(&start);
            
            #ifdef DEBUG    
            printf("AUTHORIZATION ENGINE %d MSG: %s\n",id + 1 ,msg);
            #endif

            user aux;

            // Copy the original string
            char buffer[TAM];                              
            strncpy(buffer, msg, sizeof(buffer));   
            buffer[sizeof(buffer) - 1] = '\0';              

            char *token = strtok(buffer, "#");   //Token to get the values

            if (atoi(token) == 1){ // MESSAGE FROM BACK OFFICE

                #ifdef DEBUG
                printf("MENSAGEM BACKOFFICE %s\n", msg);                
                #endif

                if (strcmp(msg, "1#data_stats") == 0){

                    sem_wait(stats_sem);

                    char tam[TAM];
                    sprintf(tam,    "SERVIÇO   TOTAL DATA  AUTH REQS\nVIDEO       %d          %d\nMUSIC       %d          %d\nSOCIAL      %d          %d\n", 
                    shm->stats.td_video, shm->stats.ar_video, shm->stats.td_music, shm->stats.ar_music, shm->stats.td_social, shm->stats.ar_social);

                    sem_post(stats_sem);
                
                    queue_msg back_msg;
                    long prty = 1;
                    back_msg.priority = prty;
                    strcpy(back_msg.message, tam);

                    if (msgsnd(mq_id, &back_msg, sizeof(queue_msg) - sizeof(long), 0) == -1) write_log("ERROR WRITING FOR MESSAGE QUEUE");

                    char log[124];
                    sprintf(log, "AUTHORIZATION_ENGINE %d: VIDEO AUTHORIZATION REQUEST (ID = 1) PROCESSING COMPLETED", id + 1);
                    write_log(log);

                } else {    
                    
                    sem_wait(stats_sem);

                    shm->stats.ar_music = 0;
                    shm->stats.ar_social = 0;
                    shm->stats.ar_video = 0;
                    shm->stats.td_music = 0;
                    shm->stats.td_video = 0;
                    shm->stats.td_social = 0;

                    sem_post(stats_sem);

                    queue_msg back_msg;
                    long prty = 1;
                    back_msg.priority = prty;
                    
                    strcpy(back_msg.message, "STATISTIC RESETED\n");
                    write_log("STATISTIC RESETED");

                    if (msgsnd(mq_id, &back_msg, sizeof(queue_msg) - sizeof(long), 0) == -1) write_log("ERROR WRITING FOR MESSAGE QUEUE");
                    
                    char log[124];
                    sprintf(log, "AUTHORIZATION_ENGINE %d: VIDEO AUTHORIZATION REQUEST (ID = 1) PROCESSING COMPLETED", id + 1);
                    write_log(log);
                }

            } else { // MESSSAGE FROM MOBILE USER

                aux.id = atoi(token);

                if (verify_user_list_full() == 0){ // IF LIST IS NOT FULL

                    if (user_in_list(aux.id) == 0){ // IF USER IS NOT IN THE LIST
                        token = strtok(NULL, "#");                
                        int valor = atoi(token);
                        if (valor != 0){

                            aux.initial_plafond = valor;
                            aux.current_plafond = valor;
                            aux.dados_reservar = -1;

                            add_user_to_list(aux);  // ADD USER TO THE LIST
                        }
                    } else if (user_in_list(aux.id) == 1) { //IS IS ALREADY IN THE LIST

                        token = strtok(NULL, "#");
                        char type[TAM];
                        strcpy(type, token); // save the type of the message (video, social or music)

                        if (is_dados_reservar_zero(aux.id) == 1){ // if aux.dados_reserver == -1

                            if ((strcmp(token, "VIDEO") == 0) || (strcmp(token, "SOCIAL") == 0) || (strcmp(token, "MUSIC") == 0)){
                                token = strtok(NULL, "#"); 
                                add_to_dados_reservar(aux.id, atoi(token)); // ADD TO "dados_reservar"

                            }
                        }

                        int plafond = update_plafond(aux.id, type);
                        
                        // send the signal to the monitor engine
                        sem_post(monitor_sem);

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

                    #ifdef LIST
                    print_user_list();
                    #endif 

                } else { // if the list is full

                    write_log("USER LIST IS FULL!");
                    queue_msg msg;

                    long prty = aux.id;
                    msg.priority = prty;
                    strcpy(msg.message, "USER LIST IS FULL!");
                    write_log("USER LIST IS FULL!");

                    if (msgsnd(mq_id, &msg, sizeof(queue_msg) - sizeof(long), 0) == -1){
                        write_log("CANNOT WRITE FOR MESSAGE QUEUE");
                    }
                }
            }

            time(&end);

            double elapsed = difftime(end, start);
            if (elapsed/1000 < config->auth_proc_time){
                sleep((config->auth_proc_time - elapsed) / 1000);
            }

            sem_wait(autho_free);
            shm->authorization_free[id] = 0;
            sem_post(autho_free);

        } else {
            perror("ERROR READING FROM THE PIPE");
        }
        memset(msg, 0, sizeof(msg));
    }
}

// Authorization request manager function
void authorization_request_manager(){

    signal(SIGINT, author_signal);

    pid_t main_pid = getpid();

    // Create named pipes
    create_named_pipe(USER_PIPE);
    create_named_pipe(BACK_PIPE);

    init_unnamed_pipes();

    // Create authorization engine processes
    for (int i = 0; i < config->auth_servers + 1; i++){
        if (main_pid == getpid()){
            pid_t ae = fork();
            if (ae == 0){
                authorization_engine(i);
                exit(0);
            } else if (ae == -1) {
                write_log("ERROR CREATING AUTHORIZATION ENGINE");
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
}

// ============= MAIN PROGRAM =============

// Authorization Engine signal handler
void author_signal(int sig){

        pthread_cancel(receiver_thread);
        pthread_cancel(sender_thread);

        while (wait(NULL) > 0);

        write_Queue(queue_other);
        write_Queue(queue_video);

        destroyQueue(queue_other);
        destroyQueue(queue_video);

        exit(0);
}

// Monitor signal handler
void monitor_signal(int sig){

        pthread_cancel(back_office_stats);
        pthread_cancel(monitor_warning);
        
        exit(0);
}

// Closing function
void cleanup(int sig){

    if (getpid() == system_manager_process){

        write_log("SIGNAL SIGINT RECEIVED");

        int status, status1;

        sem_wait(running_sem);
        shm->running = 0;
        sem_post(running_sem);

        write_log("5G_AUTH_PLATFORM SIMULATOR WAITING FOR LAST TASKS TO FINISH");

        // Wait for Authorization and Monitor engine
        if (waitpid(authorization_request_manager_process, &status, 0) == -1 ) write_log("ERROR IN WAITPID\n");
        if (waitpid(monitor_engine_process, &status1, 0) == -1) write_log("ERROR IN WAITPID\n");

        write_log("5G_AUTH_PLATFORM SIMULATOR CLOSING");

        // Close Message Queue
        if (msgctl(mq_id, IPC_RMID, NULL) == -1) write_log("ERROR CLOSING P MESSAGE QUEUE\n");

        pthread_mutex_destroy(&video_mutex);
        pthread_mutex_destroy(&other_mutex);
        pthread_cond_destroy(&sender_cond);

        // Close log file, destroy sem and pipes
        if (sem_close(log_semaphore) == -1) write_log("ERROR CLOSING LOG SEMAPHORE\n");
        if (sem_unlink(LOG_SEM_NAME) == -1 ) write_log ("ERROR UNLINKING LOG SEMAPHORE\n");
        if (sem_close(user_sem) == -1) write_log("ERROR CLOSING USER SEMAPHORE\n");
        if (sem_unlink(USER_SEM) == -1 ) write_log ("ERROR UNLINKING USER SEMAPHORE\n");
        if (sem_close(autho_free) == -1) write_log("ERROR CLOSING AUTHORIZATION ENGINE SEMAPHORE\n");
        if (sem_unlink(AUTH_SEM) == -1 ) write_log ("ERROR UNLINKING AUTHORIZATION ENGINE SEMAPHORE\n");
        if (sem_close(stats_sem) == -1) write_log("ERROR CLOSING STATS SEMAPHORE\n");
        if (sem_unlink(STATS_SEM) == -1 ) write_log ("ERROR UNLINKING STATS SEMAPHORE\n");
        if (sem_close(monitor_sem) == -1) write_log("ERROR CLOSING MONITOR SEMAPHORE\n");
        if (sem_unlink(MON_SEM) == -1 ) write_log ("ERROR UNLINKING MONITOR SEMAPHORE\n");
        if (sem_close(running_sem) == -1) write_log("ERROR CLOSING RUNNING SEMAPHORE\n");
        if (sem_unlink(RUN_SEM) == -1 ) write_log ("ERROR UNLINKING RUNNING SEMAPHORE\n");
        
        if (fclose(log_file) == EOF) write_log("ERROR CLOSIGN LOG FILE\n");
        if (unlink(USER_PIPE) == -1) write_log("ERROR UNLINKING PIPE USER_PIPE\n");
        if (unlink(BACK_PIPE) == -1 )write_log("ERROR UNLINKING PIPE BACK_PIPE\n");

        //Delete shared memory
        if(shmdt(shm)== -1) write_log("ERROR IN shmdt");
        if(shmctl(shm_id, IPC_RMID, NULL) == -1) write_log("ERROR IN shmctl");

        //Free config malloc
        free(config);

        exit(0);
    }
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
        write_log("ERRO CREATING MESSAGE QUEUE\n");
        exit(1);
    } 

    FILE *fp = fopen(MSQ_FILE, "w");
    if (fp == NULL) {
        write_log("ERROR OPENING FILE -> MSG_QUEUE_ID\n");
        exit(1);
    }
    fprintf(fp, "%d", mq_id);
    fclose(fp);

    //Create de shared memory
	int shsize= (sizeof(shared_m)  + sizeof(user)*config->max_mobile_users + sizeof(int) * (config->auth_servers + 1)); // SM size
	shm_id = shmget(IPC_PRIVATE, shsize, IPC_CREAT | IPC_EXCL | 0777);
	if(shm_id==-1){
		write_log("Erro no shmget\n");
		exit(1);
	}

	if((shm = (shared_m *) shmat(shm_id, NULL,0)) == (shared_m *) - 1){
		write_log("Erro shmat\n");
		exit(0);
	}

    shm->mem = (user *)(shm + 1);
    shm->authorization_free = (int *)(shm->mem + config->max_mobile_users);

    for (int i = 0; i < config->max_mobile_users; i++){
        shm->mem[i] = (user){.initial_plafond = -999,
                            .current_plafond = -999,
                            .dados_reservar = -999,
                            .id = -999,
                            .index = -999,
                            .triggers = 0}; 
    }

    for (int i = 0; i < config->auth_servers + 1; i++) shm->authorization_free[i] = 0;

    shm->running = 1;

    shm->stats.ar_music = 0;
    shm->stats.ar_social = 0;
    shm->stats.ar_video = 0;
    shm->stats.td_music = 0;
    shm->stats.td_video = 0;
    shm->stats.td_social = 0;

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

    sem_unlink(STATS_SEM);
    stats_sem = sem_open(STATS_SEM, O_CREAT | O_EXCL, 0777, 1);
    if (stats_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_unlink(MON_SEM);
    monitor_sem = sem_open(MON_SEM, O_CREAT | O_EXCL, 0777, 0);
    if (monitor_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }    

    sem_unlink(RUN_SEM);
    running_sem = sem_open(RUN_SEM, O_CREAT | O_EXCL, 0777, 1);
    if (running_sem == SEM_FAILED) {
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

    system_manager_process = getpid();

    // Initialize log file and log sempahore
    init_log();

    // Initialize program
    init_program();

    // Signal to end program
    signal(SIGINT, cleanup);

    wait(NULL);
}