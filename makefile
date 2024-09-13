CC = gcc
CFLAGS = -Wall -pedantic
CPPFLAGS = -pthread

main_dir = .
src = $(main_dir)/src
headers = $(src)/headers
test_dir = $(main_dir)/test
tmp_dir = $(main_dir)/tmp

#codice sorgente .c
source_c := $(wildcard $(src)/*.c)
#file .o creati a partire dal suddetto codice sorgente
objects := $(patsubst $(src)/%.c, %.o, $(source_c))
#esclude collector, si limita ai file del processo master
obj_master := $(filter-out collector.o, $(objects))

#lista di chiamate phony
.PHONY: all maketest test debug clean clean_test_dir clean_tmp_dir

#genera tutto il necessario per l'esecuzione del programma
all: maketest farm

#debug esegue all con un flag aggiuntivo che esegue delle stampe di controllo
debug: CPPFLAGS += -DDEBUG
debug: all

#crea l'eseguibile del programma
farm: $(obj_master) collector
	$(CC) $(CFLAGS) $(obj_master) -o $@

#creazione delle build .o
main.o: $(src)/main.c $(headers)/masterworker.h $(headers)/utils.h
	$(CC) $(CFLAGS) $< -c

masterworker.o: $(src)/masterworker.c $(headers)/masterworker.h $(headers)/pool.h $(headers)/utils.h
	$(CC) $(CFLAGS) $< -c

pool.o: $(src)/pool.c $(headers)/message.h $(headers)/pool.h $(headers)/utils.h $(headers)/workfun.h
	$(CC) $(CFLAGS) $< -c

workfun.o: $(src)/workfun.c $(headers)/utils.h
	$(CC) $(CFLAGS) $< -c
	
collector: $(src)/collector.c $(headers)/message.h $(headers)/utils.h
	$(CC) $(CFLAGS) $< -o $@

maketest: generafile

#compila l'eseguibile di generafile
generafile: $(test_dir)/generafile.o
	$(CC) $^ -o $@

#build .o di generafile
generafile.o: $(test_dir)/generafile.c
	$(CC) $(CFLAGS) $^ -c $@

#rimuove gli eseguibili dei due processi principali e 'chiama' la pulizia delle subdirectory
clean: clean_test_dir clean_tmp_dir
	-rm farm
	-rm collector
	-rm *.o

#rimuove i file creati durante l'esecuzione
clean_test_dir:
	-rm generafile
	-rm *.dat
	-rm expected.txt
	-rm -f -R testdir
	
#rimuove tutti i file nella cartella tmp
clean_tmp_dir:
	-rm $(tmp_dir)/*

#esecuzione dello script di test
test:
	test/test.sh
