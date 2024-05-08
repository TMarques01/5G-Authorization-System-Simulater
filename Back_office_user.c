#include "shared_memory.h"
#include "System_manager.h"

int fd_back_pipe;

void handle_signal(int sigint){
    printf("BACKOFFICE TERMINATED\n");
    exit(0);
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
    
    if ((fd_back_pipe = open(BACK_PIPE, O_WRONLY)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(0);
    }
    
    signal(SIGINT, handle_signal);

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

        sleep(2);

        queue_msg back_mq;
        msgrcv(mq_id, &back_mq, sizeof(back_mq) - sizeof(long), 0, 0);

        printf("%s", back_mq.message);

        memset(buffer, 0 , sizeof(buffer));

    
    return 0;
}