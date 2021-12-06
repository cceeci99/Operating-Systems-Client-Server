# makefile

#compiler
CC=gcc 

#flags
CFLAGS = -Wall -Werror -g
LDFLAGS = -lpthread

#object file
OBJ = os.o
#source file
SRC = os.c
#executable file
EXEC = os

$(EXEC):
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC) $(LDFLAGS)

run:
	./$(EXEC) $(file) $(k) $(n)

val:
	valgrind --leak-check=full ./$(EXEC) $(file) $(k) $(n)

time:
	time ./$(EXEC) $(file) $(k) $(n)

clean:
	rm -f  $(OBJ) $(EXEC)