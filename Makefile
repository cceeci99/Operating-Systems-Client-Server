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
	./$(EXEC) $(file) $(K) $(N)

valgrind:
	valgrind --leak-check=full ./$(EXEC) $(file) $(K) $(N)

time:
	time ./$(EXEC) $(file) $(K) $(N)

clean:
	rm -f  $(OBJ) $(EXEC)