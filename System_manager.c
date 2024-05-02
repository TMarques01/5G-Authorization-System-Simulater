//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "funcoes.h"
#define DEBUG

// Mutex
pthread_mutex_t video_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t other_mutex = PTHREAD_MUTEX_INITIALIZER;

int is_dados_reservar_zero(users_list *list, int id) {
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            return current->user.dados_reservar == -1 ? 1 : 0;
        }
        current = current->next;
    }
    return 0;  // Retorna 0 se o ID não for encontrado
}

int user_in_list(users_list *list, int id) {
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            return 1;  // ID encontrado na lista
        }
        current = current->next;
    }
    return 0;  // ID não encontrado na lista
}

void add_to_dados_reservar(users_list *list, int id, int add_value) {
    users_list *current = list;
    while (current != NULL) {
        if (current->user.id == id) {
            current->user.dados_reservar = add_value;  // Adiciona o valor a "dados_reservar"
            return;  // Retorna após adicionar o valor
        }
        current = current->next;
    }
}

int update_plafond(shared_m *shared_data, int user_id) {
    users_list *current = shared_data->mem;  // Ponteiro para a cabeça da lista
    while (current != NULL) {
        if (current->user.id == user_id) {  // Verifica se é o usuário correto
            if (current->user.current_plafond >= current->user.dados_reservar) {
                current->user.current_plafond -= current->user.dados_reservar;  // Subtrai dados_reservar
                return 0;
            } else {
                return 1;
            }
        }
        current = current->next;  // Move para o próximo elemento na lista
    }
    return -1;
}

// ============= PROCESSES AND THREADS =============

// Monitor engine process function
void monitor_engine(){
}

// Receiver funcion
void *receiver(void *arg){
    write_log("THREAD RECEIVER CREATED");

    struct DispatcherArgs *args = (struct DispatcherArgs*) arg;
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
    if ((fd_back_pipe = open(BACK_PIPE, O_RDONLY )) < 0) {
        perror("CANNOT OPEN BACK_PIPE FOR READING\nS");
        exit(1);
    }
    
    int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
    max_fd += 1;  // Adiciona 1 ao maior descritor de arquivo
    */
    while (1) {

        // Open the FD
        FD_ZERO(&read_set);
        FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if (select(fd_user_pipe + 1, &read_set, NULL, NULL, NULL) > 0){
            if (FD_ISSET(fd_user_pipe, &read_set)){

                char *user_buffer;
                user_buffer = read_from_pipe(fd_user_pipe); 
 
                // Copy the original string
                char buffer[1024];                              
                strncpy(buffer, user_buffer, sizeof(buffer));   
                buffer[sizeof(buffer) - 1] = '\0';              

                char *token = strtok(buffer, "#");   //Token to get the values

                token = strtok(NULL, "#"); 

                if ((strcmp(token, "MUSIC") == 0) || (strcmp(token, "OTHER") == 0) || (strcmp(token, "VIDEO") == 0)){
                    if (strcmp(token, "VIDEO") == 0) { // GO TO VIDEO QUEUE

                        pthread_mutex_lock(&video_mutex);

                        enqueue(queue_video, user_buffer);
                        #ifdef DEBUG
                        printf("VIDEO USER BUFFER %s\n", user_buffer); 
                        #endif
                        pthread_mutex_unlock(&video_mutex);

                    } else { // GO TO OTHER QUEUE

                        pthread_mutex_lock(&other_mutex);

                        enqueue(queue_other, user_buffer);
                        #ifdef DEBUG
                        printf("OTHER USER BUFFER %s\n", user_buffer); 
                        #endif
                        pthread_mutex_unlock(&other_mutex);

                    }

                } else { // GO TO OTHER QUEUE

                    pthread_mutex_lock(&other_mutex);

                    enqueue(queue_other, user_buffer);
                    #ifdef DEBUG
                    printf("FIRST USER BUFFER %s\n", user_buffer); 
                    #endif
                    pthread_mutex_unlock(&other_mutex);

                }
                #ifdef DEBUG
                printf("VIDEO QUEUE SIZE %d\n", queue_size(queue_video));
                printf("OTHER QUEUE SIZE %d\n", queue_size(queue_other));
                #endif
                memset(buffer, 0 ,sizeof(buffer));
                FD_CLR(fd_user_pipe, &read_set); 

            }

            /*
            if (FD_ISSET(fd_back_pipe, &read_set)){
                char *back_buffer;
                back_buffer = read_from_pipe(fd_back_pipe);


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

    struct DispatcherArgs *args = (struct DispatcherArgs*) arg;
    //int (*pipes)[2] = args->pipes;
    struct Queue* queue_video = args->queue_video;
    struct Queue* queue_other = args->queue_other;

    while (1){

        pthread_mutex_lock(&video_mutex);

        if (!isEmpty(queue_video)){
            while(!isEmpty(queue_video)){
                char *msg_video = dequeue(queue_video);
                printf("VIDEO: %s\n", msg_video);

                /*for (int i = 0; i < config->auth_servers; i++){

                    sem_wait(autho_free);

                    if (shm->authorization_free[i] == 0){
                        write(pipes[i][1], &msg_video, sizeof(msg_video));
                        break;
                    }

                    sem_post(autho_free);

                }*/      
            }

            //pthread_mutex_unlock(&video_mutex);
        } else {

            //pthread_mutex_lock(&other_mutex);
            if (!isEmpty(queue_other)){
                while(!isEmpty(queue_other)){
                    char *msg_other = dequeue(queue_other);
                    printf("OTHER: %s\n", msg_other);

                    /*for (int i = 0; i < config->auth_servers; i++){

                        sem_wait(autho_free);

                        if (shm->authorization_free[i] == 0){
                            write(pipes[i][1], &msg_other, sizeof(msg_other));
                        }

                        sem_post(autho_free);
                    }*/
                }
                //pthread_mutex_unlock(&other_mutex);
            }
            
        }
        
    }

    pthread_exit(NULL);
} 

// Autorization engines
void authorization_engine(int id, int pipes[2]){
    write_log("AUTHORIZATION ENGINE INIT");
    
    close(pipes[1]);

    char *msg;

    read(pipes[0], &msg, sizeof(msg));

    sem_wait(autho_free);

    shm->authorization_free[id] = 1;

    sem_post(autho_free);

    sem_wait(user_sem);

    // Verify if user is in the linked list
    if (user_in_list(shm->mem, queue.current_user.id) == 0){
        printf("dentro 1\n");
        queue.current_user.current_plafond = queue.current_user.initial_plafond;
        add_user_to_list(queue.current_user);

    } else if (user_in_list(shm->mem, queue.current_user.id) == 1){
        printf("dentro 2\n");
        if (is_dados_reservar_zero(shm->mem, queue.current_user.id) == 1){ // if queue.current_user.dados_reservar == -1
            add_to_dados_reservar(shm->mem, queue.current_user.id, queue.current_user.dados_reservar);
            printf("dentro 3\n");

        } 
        
        if (queue.priority > -1){
            printf("dentro 4\n");
            int aux = update_plafond(shm, queue.current_user.id);
            if (aux == 1){ // if plafond used
                kill(queue.current_user.id, SIGINT);
            } else if (aux == -1){
                write_log("USER DONT EXIST");
                exit(1);
            }

        } 
        //else {

        //}

    }

    #ifdef DEBUG
    print_user_list();
    #endif 


    sem_post(user_sem);

    #ifdef DEBUG
    printf("AUTHORIZATION: ID %d, Current plafond %d \n", queue.current_user.id, queue.current_user.current_plafond);
    #endif 
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

    // Ceeate authorization engine processes
    for (int i = 0; i < config->auth_servers; i++){
        if (main_pid == getpid()){
            if (fork() == 0){
                authorization_engine(i, pipes[i]);
                exit(0);
            }
        }
    }

    // Create Queues
    queue_video = createQueue();
    queue_other = createQueue();

    struct DispatcherArgs dispatcher_args = { .pipes = pipes, .queue_video = queue_video, .queue_other = queue_other };

    // Create Threads
    if (pthread_create(&receiver_thread, NULL, receiver, (void *) &dispatcher_args) != 0) {
        printf("CANNOT CREATE RECEIVER_THREAD\n");
        exit(1);
    }

    if (pthread_create(&sender_thread, NULL, sender, (void*) &dispatcher_args) != 0) {
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

    //destroyQueue(queue_other);
    //destroyQueue(queue_video);

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

    while(1){}
}