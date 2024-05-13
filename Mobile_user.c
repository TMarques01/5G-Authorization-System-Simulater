//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "shared_memory.h"

int fd_named_pipe, login = 0, max_request, video_time, music_time, social_time, run = 1;
user user_data;
pthread_t video, music, social, message_queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_signal(int sigint){

    run = 0;
    printf("LOSING MOBILE USER...\n");
    pthread_cancel(video);
    pthread_cancel(social);
    pthread_cancel(music);
    pthread_cancel(message_queue);
    pthread_mutex_destroy(&mutex);

    exit(0);
}

// ============= DATA VERIFICATION FUNCTIONS =============

int verify_max_request(){

    pthread_mutex_lock(&mutex);
    if (max_request != 0){
        pthread_mutex_unlock(&mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&mutex);
        printf("MAX REQUEST OVERPAST\n");
        run = 0;
        return 0;
    }
}

int verify_data(int argc, char **argv) {
    for (int i = 1; i < argc; i++){ // Começa em 1 para ignorar o nome do programa
        for (int j = 0; argv[i][j] != '\0'; j++) {
            if (!isdigit(argv[i][j])) {
                printf("Dados incorretos!\n");
                return 0;
            }
        }
    }
    return 1;
}

void *video_thread(void *arg){

    char buffer_login[256];
    while (verify_max_request()){

        sprintf(buffer_login, "%d#VIDEO#%d", getpid(), user_data.dados_reservar);
        //printf("VIDEO QUEUE: %s\n", buffer_login);

        pthread_mutex_lock(&mutex);
        ssize_t bytes_read = write(fd_named_pipe, buffer_login, sizeof(buffer_login));
        if (bytes_read < 0){
            pthread_mutex_unlock(&mutex);
            perror("ERROR WRITING FOR USER_PIPE\n");
        } else {

            max_request--;
            pthread_mutex_unlock(&mutex);

            sleep(video_time/1000);

            memset(buffer_login, 0, sizeof(buffer_login));
        }
    }
    pthread_exit(NULL);
}

void *music_thread(void *arg){

    char buffer_login[256];
    while (verify_max_request()){

        sprintf(buffer_login, "%d#MUSIC#%d", getpid(), user_data.dados_reservar);
        //printf("MUSIC QUEUE: %s\n", buffer_login);

        pthread_mutex_lock(&mutex);
        ssize_t bytes_read = write(fd_named_pipe, buffer_login, sizeof(buffer_login));
        if (bytes_read < 0){
            pthread_mutex_unlock(&mutex);
            perror("ERROR WRITING FOR USER_PIPE\n");
        } else {

            max_request--;
            pthread_mutex_unlock(&mutex);

            sleep(music_time/1000);
            memset(buffer_login, 0, sizeof(buffer_login));
        }       
    }
    pthread_exit(NULL);
}

void *social_thread(void *arg){

    char buffer_login[256];
    while (verify_max_request()){

        sprintf(buffer_login, "%d#SOCIAL#%d", getpid(), user_data.dados_reservar);
        //printf("SOCIAL QUEUE: %s\n", buffer_login);

        pthread_mutex_lock(&mutex);
        ssize_t bytes_read = write(fd_named_pipe, buffer_login, sizeof(buffer_login));
        if (bytes_read < 0){
            pthread_mutex_unlock(&mutex);
            perror("ERROR WRITING FOR USER_PIPE\n");

        } else {
            
            max_request--;
            pthread_mutex_unlock(&mutex);

            sleep(social_time/1000);

            memset(buffer_login, 0, sizeof(buffer_login));
        }
    }
    
    pthread_exit(NULL);
}

void *message_receiver(void *arg){

    char msg_id[64];

    FILE *fp = fopen(MSQ_FILE, "r");
    if (fp == NULL){
        printf("ERROR OPENING FILE -> MSG_QUEUE_ID\n");
        exit(1);
    }
    fgets(msg_id, 64, fp);
    fclose(fp);

    while (run){
        queue_msg msg;

        if (msgrcv(atoi(msg_id), &msg, sizeof(queue_msg) - sizeof(long), getpid(), 0) == -1){
            perror("ERROR RECEIVING FROM MQ");
            exit(1);
        }

        printf("%s\n", msg.message);
        
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]){

    if (argc != 7){
        printf("INCORRECT NUMBER OF ARGUMENTS\n");
        exit(0);
    }
    
    if (verify_data(argc, argv) != 0){
        user_data.initial_plafond = atoi(argv[1]);
        max_request = atoi(argv[2]);
        video_time = atoi(argv[3]);
        music_time = atoi(argv[4]);
        social_time = atoi(argv[5]);
        user_data.dados_reservar = atoi(argv[6]);
        user_data.id = getpid();

    } else {
        printf("WRONG DATA\n");
        exit(1);
    }

    signal(SIGINT, handle_signal);

    if ((fd_named_pipe = open(USER_PIPE, O_RDWR)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(1);
    }
    
    while (run){

        signal(SIGINT, handle_signal);
        
        if (login == 0){
            
            char buffer[256];
            sprintf(buffer, "%d#%d", getpid(), user_data.initial_plafond);
            printf("LOGIN %s\n", buffer);

            ssize_t bytes_read = write(fd_named_pipe, buffer, sizeof(buffer));
            if (bytes_read < 0){
                perror("ERROR WRITING FOR PIPE");
            }

            login++;
            memset(buffer, 0 , sizeof(buffer));

        } else if (login == 1){
            
            // Create Threads
            if (pthread_create(&video, NULL, video_thread, NULL) != 0) {
                printf("CANNOT CREATE VIDEO_THREAD\n");
                exit(1);
            }

            if (pthread_create(&music, NULL, music_thread, NULL) != 0) {
                printf("CANNOT CREATE MUSIC_THREAD\n");
                exit(1);
            }

            if (pthread_create(&social, NULL, social_thread, NULL) != 0) {
                printf("CANNOT CREATE SOCIAL_THREAD\n");
                exit(1);
            }

            if (pthread_create(&message_queue, NULL, message_receiver, NULL) != 0){
                printf("CANNOT CREATE MESSAGE_THREAD\n");
                exit(1);
            }
                // Closing threads
            if (pthread_join(video, NULL) != 0){
                printf("CANNOT JOIN VIDEO THREAD");
                exit(1);
            }

            if (pthread_join(music, NULL) != 0){
                printf("CANNOT JOIN MUSIC THREAD");
                exit(1);
            }
            
            if (pthread_join(social, NULL) != 0){
                printf("CANNOT JOIN SOCIAL THREAD");
                exit(1);
            }

            if (pthread_join(message_queue, NULL) != 0){
                printf("CANNOT JOIN MESSAGE QUEUE THREAD");
                exit(1);
            }
        }
    }
    return 0; 
}