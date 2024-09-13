CC = gcc
CFLAGS = -Wall -pedantic

main_dir = .
src = $(main_dir)/src
headers = $(src)/headers
build_dir = $(main_dir)/build
test_dir = $(main_dir)/test
tmp_dir = $(main_dir)/tmp

#codice sorgente .c
source_c := $(wildcard $(src)/*.c)
#file .o creati a partire dal suddetto codice sorgente
objects := $(patsubst $(src)/%.c, $(build_dir)/%.o, $(source_c))

.PHONY: all test clean clean_build_dir clean_test_dir clean_tmp_dir

all: generafile farm


#compila generafile
generafile: $(test_dir)/generafile.c
	$(CC) $(CFLAGS) $^ -o $@

farm: $(objects)
	$(CC) $(CFLAGS) $(objects) -o $@

#creazione delle build .o
$(build_dir)/main.o: $(headers)/masterworker.h $(headers)/utils.h
	$(CC) $(CFLAGS) $@ -c

$(build_dir)/masterworker.o: $(headers)/masterworker.h $(headers)/pool.h $(headers)/utils.h
	$(CC) $(CFLAGS) $@ -c

$(build_dir)/pool.o: $(headers)/message.h $(headers)/pool.h $(headers)/workfun.h
	$(CC) $(CFLAGS) $@ -c

$(build_dir)/workfun.o: $(headers)/utils.h
	$(CC) $(CFLAGS) $@ -c

#rimuove gli eseguibili dei due processi principali e 'chiama' la pulizia delle subdirectory
clean: clean_build_dir clean_test_dir clean_tmp_dir 
	-rm farm2
	-rm collector

#rimuove i file .o
clean_build_dir:
	-rm $(build_dir)/*.o

#rimuove i file creati durante l'esecuzione
clean_test_dir:
	-rm generafile
	-rm *.dat
	-rm -f -R testdir
	
#rimuove tutti i file nella cartella tmp
clean_tmp_dir:
	-rm $(tmp_dir)/*
