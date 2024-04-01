//Mariana Sousa 2022215999
//Tiago Marques 2022210638

#include "shared_memory.h"


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

int main(int argc, char* argv[]){

    user user_data;

    if (argc != 7){
        printf("Número incorreto de argumentos\n");
        exit(0);
    }
    
    if (verify_data(argc, argv) != 0){
        user_data.initial_plafond = atoi(argv[1]);
        user_data.max_request = atoi(argv[2]);
        user_data.video = atoi(argv[3]);
        user_data.video = atoi(argv[4]);
        user_data.social = atoi(argv[5]);
        user_data.dados_reservar = atoi(argv[6]);
    }
    
}