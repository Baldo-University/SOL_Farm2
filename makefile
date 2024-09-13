CC = gcc
CFLAGS = -Wall -pedantic

main_dir = .					#directory del progetto
src = $(main_dir)/src			#directory del source code
headers = $(src)/headers		#directory degli header
build_dir = $(main_dir)/build	#directory contenente i *.o
test_dir = $(main_dir)/test		#directory per i test
tmp_dir = $(main_dir)/tmp		#directory per i file temporanei

#codice sorgente .c
source_c := $(wildcard $(src)/*.c)
#file .o creati a partire dal suddetto codice sorgente
objects := $(patsubst $(src)/%.c, $(src)/%.0, $(source_c))

.PHONY: all test clean clean_build_dir clean_test_dir clean_tmp_dir

all:
	

test:
	test/test.sh

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
