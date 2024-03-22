#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

typedef struct program_init{
    int MAX_MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT; 
} program_init;
 

int is_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0; // Não é um número
        }
    }
    return 1; // É um número
}

int file_verification(program_init *programa) {
    FILE *f = fopen("config.txt", "r");
    if (f == NULL) {
        printf("Erro ao abrir o ficheiro\n");
        return -1;
    }

    char line[50];
    int count = 0;
    int temp_val;

    while (fgets(line, sizeof(line), f)) {
        // Verifica se a linha contém um número
        line[strcspn(line, "\n")] = '\0';
        if (is_number(line)) {
            temp_val = atoi(line);
            // Verifica se o valor está dentro dos critérios esperados
            if (temp_val < 0 || ((count == 2 || count == 4 || count == 5) && temp_val < 1)) {
                printf("Valores incorretos no ficheiro\n");
                fclose(f);
                return -2;
            }

            // Atribui o valor lido à estrutura, baseado no contador
            switch (count) {
                case 0: programa->MAX_MOBILE_USERS = temp_val; break;
                case 1: programa->QUEUE_POS = temp_val; break;
                case 2: programa->AUTH_SERVERS = temp_val; break;
                case 3: programa->AUTH_PROC_TIME = temp_val; break;
                case 4: programa->MAX_VIDEO_WAIT = temp_val; break;
                case 5: programa->MAX_OTHERS_WAIT = temp_val; break;
                default:
                    fclose(f);
                    return -3; // Número inesperado de linhas
            }
            count++;
        } else {
            printf("Linha não é um número válido.\n");
            fclose(f);
            return -2;
        }
    }

    fclose(f);

    if (count != 6) {
        printf("Número insuficiente de dados!\n");
        return -3;
    }

    return 0; // Sucesso
}


void monitor_engine(){
    sleep(2);
}

void autorization_request_manager(){
    sleep(2);
}

int main(int argc, char* argv[]){

    program_init program;

    if (file_verification(&program) != 0){
        return 0;
    }


    // Criação do processo Monitor engine
    int monitor_engine_process = fork();
    if (monitor_engine_process == 0){
        monitor_engine();
        exit(0);
    } else if (monitor_engine_process == -1){
        perror("Erro na criação do processo (Monitor engine)\n");
        exit(0);
    }

    // Criação do processo Autorization Requeste Manager
    int autorization_request_manager_process = fork();
    if (autorization_request_manager_process == 0){
        autorization_request_manager();
        exit(0);
    } else if (autorization_request_manager_process == -1){
        perror("Erro na criação do processo (Autorization Request Manager)\n");
        exit(0);
    }

    // Funeral dos processos
    for(int i=0;i<2;i++){
		wait(NULL);
	}



}