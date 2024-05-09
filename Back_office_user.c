#include "shared_memory.h"
#include "System_manager.h"

int fd_back_pipe;
int run = 1;

void handle_signal(int sigint){
    run = 0;
    sleep(1);
    close(fd_back_pipe);
    printf("\nBACKOFFICE TERMINATED\n");

    exit(0);
}

void *receive_stats(void *args){

    char *msg_id = (char*)args;

    while(run){

        queue_msg back_mq;
        if (msgrcv(atoi(msg_id), &back_mq, sizeof(queue_msg) - sizeof(long), 1, 0) == -1){
            perror("ERROR RECEIVING FROM MQ");
            exit(1);
        }

        printf("Statistics\n%s", back_mq.message);

    }

    pthread_exit(NULL);
}

int check_command(const char *input) {
    // Verifica se a entrada é igual a "1#data_stats"
    if (strcmp(input, "1#data_stats") == 0) {
        return 1;
    }
    // Verifica se a entrada é igual a "1#reset"
    else if (strcmp(input, "1#reset") == 0) {
        return 1;
    }
    // Retorna 0 se a entrada não corresponder a nenhum dos comandos esperados
    return 0;
}

int main(){

    char msg_id[64];
    if ((fd_back_pipe = open(BACK_PIPE, O_WRONLY)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(0);
    }

    FILE *fp = fopen(MSQ_FILE, "r");
    if (fp == NULL){
        printf("ERROR OPENING FILE -> MSG_QUEUE_ID\n");
        exit(1);
    }
    fgets(msg_id, 64, fp);
    fclose(fp);

    pthread_t back_office_stats;

    if (pthread_create(&back_office_stats, NULL, receive_stats,(void*)msg_id) != 0) {
        printf("CANNOT CREATE BACK_OFFICE_STATS_THREAD\n");
        exit(1);
    }
    
    signal(SIGINT, handle_signal);

    while(run){

        char buffer[256];        
        fgets(buffer, 256, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (check_command(buffer) != 1){ // Verify data
            printf("VALORES ERRADOS\n");
            exit(1);
        }

        ssize_t bytes_write = write(fd_back_pipe, buffer, sizeof(buffer));
        if (bytes_write < 0){
            perror("CANNOT WRITE FOR PIPE\n");
            exit(1);
        } else {
            printf("SENDED\n");
        }

        memset(buffer, 0 , sizeof(buffer));
    } 

    // Closing threads
    if (pthread_join(back_office_stats, NULL) != 0){
        printf("CANNOT JOIN BACK OFFICE STATS THREAD\n");
        exit(1);
    }
}