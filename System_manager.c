//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "System_manager.h"
#include "funcoes.h"

// ============= PROCESSES AND THREADS =============

// Monitor engine process function
void monitor_engine(){
}

// Receiver funcion
void *receiver(void *arg){
    write_log("THREAD RECEIVER CREATED");

    int fd_user_pipe, fd_back_pipe;
    fd_set read_set;

    // Named pipes for reading
    if ((fd_user_pipe = open(USER_PIPE, O_RDONLY )) < 0) {
        perror("CANNOT OPEN USER_PIPE FOR READING\nS");
        exit(1);
    }

    if ((fd_back_pipe = open(BACK_PIPE, O_RDONLY )) < 0) {
        perror("CANNOT OPEN BACK_PIPE FOR READING\nS");
        exit(1);
    }

    int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
    max_fd += 1;  // Adiciona 1 ao maior descritor de arquivo

    while (1){

        // Open the FD
        FD_ZERO(&read_set);
        FD_SET(fd_user_pipe, &read_set);
        FD_SET(fd_back_pipe, &read_set);

        if (select(max_fd, &read_set, NULL, NULL, NULL) > 0){
            if (FD_ISSET(fd_user_pipe, &read_set)){
                char *user_buffer;
                user_buffer = read_from_pipe(fd_user_pipe);   



                FD_CLR(fd_user_pipe, &read_set);       
            }
            if (FD_ISSET(fd_back_pipe, &read_set)){
                char *back_buffer;
                back_buffer = read_from_pipe(fd_back_pipe);


                FD_CLR(fd_back_pipe, &read_set);
            }


        }

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

    //mem->next = NULL;
    


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