CC = gcc
FLAGS = -Wall -pthread -g -fcommon
PROGS = 5g_auth_platform Mobile_user Back_office_user
OBJS = System_manager.o Mobile_user.o Back_office_user.o funcoes.o

all: ${PROGS}

clean:
	rm -f *.o *~ $(PROGS)

5g_auth_platform: System_manager.o funcoes.o
	${CC} ${FLAGS} $^ -o $@

Mobile_user: Mobile_user.o funcoes.o
	${CC} ${FLAGS} $^ -o $@
    
Back_office_user: Back_office_user.o funcoes.o
	${CC} ${FLAGS} $^ -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

##########################

System_manager.o: System_manager.c System_manager.h shared_memory.h funcoes.h
Mobile_user.o: Mobile_user.c shared_memory.h
Back_office_user.o: Back_office_user.c shared_memory.h
funcoes.o: funcoes.c funcoes.h
