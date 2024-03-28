#include "shared_memory.h"
 
//Função para verificar se uma string é um número
int is_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0; // Não é um número
        }
    }
    return 1; // É um número
}

//Função de verificação do ficheiro
int file_verification() {
    FILE *f = fopen("config.txt", "r");

    if (f == NULL) {
        printf("Erro ao abrir o ficheiro\n");
        return -1;
    }


    char line[50];
    int count = 0;
    int temp_val;

    config = malloc(sizeof(program_init));

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        // Verifica se a linha contém um número
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
                case 0: config->max_mobile_users = temp_val; break;
                case 1: config->queue_pos = temp_val; break;
                case 2: config->auth_servers = temp_val; break;
                case 3: config->auth_proc_time = temp_val; break;
                case 4: config->max_video_wait = temp_val; break;
                case 5: config->max_others_wait = temp_val; break;
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
    sleep(1);
}

void autorization_request_manager(){
    sleep(1);
}

// Fechar tudo que há para fechar
void cleanup(){

    //Free config malloc
    free(config);


    // Wait for 2 process
    for(int i=0;i<2;i++){
		wait(NULL);
	}
}

int main(int argc, char* argv[]){

    // Verifiar ficheiro config (valores guardados variavel "config")
    if (file_verification() != 0){
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

    //Cleaning...
    cleanup();

    return 0;
}