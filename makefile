CC = gcc
CFLAGS = -Wall -pedantic

main_objects = main.o masterworker.o pool.o workfun.o

main: $(main_objects)
	$(CC) $^ -o $@
	
collector: collector.c
	$(CC) $(CFLAGS) $^ -o $@
	
main.o: main.c
	$(CC) $(CFLAGS) $^ -c

masterworker.o: masterworker.c
	$(CC) $(CFLAGS) $^ -c

pool.o: pool.c
	$(CC) $(CFLAGS) $^ -c

workfun.o: workfun.c
	$(CC) $(CFLAGS) $^ -c
	
clean:
	-rm farm2.sck
	-rm *.o
