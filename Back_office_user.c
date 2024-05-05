#include "shared_memory.h"
#include "System_manager.h"

int fd_named_pipe;


void handle_signal(int sigint){
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


    if ((fd_named_pipe = open(BACK_PIPE, O_WRONLY)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(0);
    }
    
    while (1){
        //char buffer[256];

        signal(SIGINT, handle_signal);
        
        //fgets(buffer, 256, stdin);
        //buffer[strcspn(buffer, "\n")] = 0;

        //if (check_command(buffer) != 1){ // Verify data
        //    printf("VALORES ERRADOS\n");
        //    exit(1);
        //}

        ssize_t bytes_write = write(fd_named_pipe, "1#data_stats", sizeof("1#data_stats"));
        if (bytes_write < 0){
            perror("CANNOT WRITE FOR PIPE\n");
            exit(1);
        } else {
            printf("SENDED\n");
        }

        //memset(buffer, 0 , sizeof(buffer));
    }
    return 0;
}