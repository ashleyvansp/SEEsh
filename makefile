CC = gcc
CFLAGS = -g -Wall

SEEsh: SEEsh.c
	$(CC) $(CFLAGS) -o SEEsh SEEsh.c

SEEsh.o:  SEEsh.c SEEsh.h 
	$(CC) $(CFLAGS) -c SEEsh.c

run: SEEsh
	./SEEsh

valgrind: SEEsh
	valgrind --leak-check=full --show-leak-kinds=all ./SEEsh

clean:
	$(RM) SEEsh *.o *~
