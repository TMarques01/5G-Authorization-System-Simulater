#include "shared_memory.h"

int fd_named_pipe;
char buffer[256];

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


    if ((fd_named_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(0);
    }
    
    while (1){

        signal(SIGINT, handle_signal);
        
        fgets(buffer, 256, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (check_command(buffer) != 1){ // Verify data
            printf("VALORES ERRADOS\n");
            exit(1);
        }

        write(fd_named_pipe, buffer, sizeof(buffer));

        memset(buffer, 0 , sizeof(buffer));
    }
    return 0;
}