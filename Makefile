CC = gcc
FLAGS = -Wall -pthread -g
PROGS = 5g_auth_platform Mobile_user Back_office_user
OBJS = System_manager.o Mobile_user.o Back_office_user.o 

all: ${PROGS}

clean:
	rm -f *.o *~ $(PROGS)

5g_auth_platform: System_manager.o 
	${CC} ${FLAGS} $^ -o $@

Mobile_user: Mobile_user.o 
	${CC} ${FLAGS} $^ -o $@
    
Back_office_user: Back_office_user.o
	${CC} ${FLAGS} $^ -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

##########################

System_manager.o: System_manager.c System_manager.h shared_memory.h
Mobile_user.o: Mobile_user.c shared_memory.h System_manager.h
Back_office_user.o: Back_office_user.c shared_memory.h
