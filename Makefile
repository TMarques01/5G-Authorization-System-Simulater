CC = gcc
FLAGS = -Wall -pthread -g
PROGS = System_manager Mobile_user Back_office_user 
OBJS = System_manager.o Mobile_user.o Back_office_user.o 

all: ${PROGS}

clean:
	rm ${OBJS} *~ ${PROGS} 

System_manager: System_manager.o 
	${CC} ${FLAGS} $^ -o $@

Mobile_user: Mobile_user.o  
	${CC} ${FLAGS} $^ -o $@
    
Back_office_user: Back_office_user.o 
	${CC} ${FLAGS} $^ -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@
