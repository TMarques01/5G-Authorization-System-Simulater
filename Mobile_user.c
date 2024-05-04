//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "shared_memory.h"
#include "System_manager.h"
#include "funcoes.h"

int fd_named_pipe, login = 0;;
user user_data;


void handle_signal(int sigint){
    exit(0);
}

// ============= DATA VERIFICATION FUNCTIONS =============

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

int process_input(const char *input) {
    char *part1, *part2;
    char buffer[256];  // Buffer para armazenar a entrada de forma segura
    char *hash_pos;

    // Copia a entrada para o buffer para evitar modificar a string original
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // Encontra a posição do caractere '#'
    hash_pos = strchr(buffer, '#');
    if (hash_pos == NULL || strchr(hash_pos + 1, '#') != NULL) {
        printf("ERROR: MUST HAVE ONE '#'.\n");
        return 0;
    }

    // Separa a string em duas partes
    *hash_pos = '\0';  // Divide a string em duas, substituindo '#' por '\0'
    part1 = buffer;
    part2 = hash_pos + 1;

    // Verifica se ambas as partes são numéricas usando a função is_number
    if (!is_number(part1) || !is_number(part2)) {
        printf("ERROR: BOTH PARTS SHOULD BE NUMERIC.\n");
        return 0;
    }

    if (atoi(part1) != getpid()){
        printf("ERROR: DIFFERENT ID\n");
        return 0;
    }

    if (atoi(part2) != user_data.initial_plafond){
        printf("ERROR: DIFFERENT INITIAL PLAFOND\n");
        return 0;
    }

    // Se passar nas verificações, retorna verdadeiro
    return 1;
}

int is_valid_keyword(const char *str) {
    // Verifica se a palavra é uma das opções válidas
    return strcmp(str, "MUSIC") == 0 || strcmp(str, "VIDEO") == 0 || strcmp(str, "SOCIAL") == 0;
}

int process_input2(const char *input) {
    char buffer[256];
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // Separa a string pelas ocorrências de '#'
    char *first = strtok(buffer, "#");
    char *keyword = strtok(NULL, "#");
    char *second = strtok(NULL, "#");

    // Verifica se todos os tokens foram obtidos corretamente
    if (first == NULL || keyword == NULL || second == NULL) {
        printf("ERROR: USE TWO '#'.\n");
        return 0;
    }

    // Verifica se o primeiro e o terceiro elementos são números e o segundo é uma palavra válida
    if (!is_number(first) || !is_number(second) || !is_valid_keyword(keyword)) {
        printf("ERROR: '123#WORD#456'\n");
        return 0;
    }

    if (atoi(first) != getpid()){
        printf("ERROR: DIFFERENT ID\n");
        return 0;
    }

    if (atoi(second) != user_data.dados_reservar){
        printf("ERROR: DIFFERENT RESERVED DATA\n");
        return 0;
    }

    return 1; // A entrada é válida
}

int main(int argc, char* argv[]){

    if (argc != 7){
        printf("Número incorreto de argumentos\n");
        exit(0);
    }
    
    int video, music, social, max_request;

    if (verify_data(argc, argv) != 0){
        user_data.initial_plafond = atoi(argv[1]);
        max_request = atoi(argv[2]);
        video = atoi(argv[3]);
        music = atoi(argv[4]);
        social = atoi(argv[5]);
        user_data.dados_reservar = atoi(argv[6]);
        user_data.id = getpid();
    } else {
        printf("WRONG DATA\n");
        exit(0);
    }

    printf("YOUR ID: %d\n", getpid());

    signal(SIGINT, handle_signal);  

    if ((fd_named_pipe = open(USER_PIPE, O_WRONLY)) < 0) {
        perror("CANNOT OPEN PIPE FOR WRITING: ");
        exit(0);
    }
    
    while (1){

        signal(SIGINT, handle_signal);
        
        if (login == 0){
            char buffer[256];
            sprintf(buffer, "%d#%d", getpid(), user_data.initial_plafond);
            printf("LOGIN %s\n", buffer);

            if (process_input(buffer) != 1){ // Verify data
                printf("WRONG VALUES\n");
                exit(0);
                
            } else {
                write(fd_named_pipe, buffer, sizeof(buffer));

                login++;
                memset(buffer, 0 , sizeof(buffer));
            }



        } else if (login == 1){
                
            char aux[1024];  
            char buffer_login[1024];                                           

            sprintf(buffer_login, "%d#VIDEO#%d", getpid(), user_data.dados_reservar);
            printf("VIDEO QUEUE: %s\n", buffer_login);


            strncpy(aux, buffer_login, sizeof(aux));   
            aux[sizeof(aux) - 1] = '\0';

            char *token = strtok(aux, "#");
            token = strtok(NULL, "#");              // Taking the "MUSIC" or "SOCIAL"

            if (process_input2(buffer_login) != 1){ // Verify data
                printf("WRONG VALUES\n");
                exit(0);
                
            } else {
                // em vez de ter o for vais ter 3 thread a enviar
                for (int i = 0; i < max_request; i++){

                    write(fd_named_pipe, buffer_login, sizeof(buffer_login));
                    
                    if (strcmp(token, "MUSIC") == 0){
                        sleep(music);
                    } else if (strcmp(token, "SOCIAL") == 0){
                        sleep(social);
                    } else if (strcmp(token, "VIDEO") == 0){
                        sleep(video);
                    }
                }
                break;
            }
        }
    }
}