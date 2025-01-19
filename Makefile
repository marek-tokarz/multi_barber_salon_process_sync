all: KASJER PROCEDURA_KLIENT PROCEDURA_FRYZJER serwer_poczekalnia proces_klient proces_fryzjer

KASJER: KASJER.o operacje_semafor.o
	gcc -o KASJER KASJER.o operacje_semafor.o

KASJER.o: KASJER.c header_file.h 
	gcc -c KASJER.c

operacje_semafor.o: operacje_semafor.c
	gcc -c operacje_semafor.c

PROCEDURA_KLIENT: PROCEDURA_KLIENT.o operacje_semafor.o
	gcc -o PROCEDURA_KLIENT PROCEDURA_KLIENT.o operacje_semafor.o

PROCEDURA_FRYZJER: PROCEDURA_FRYZJER.o operacje_semafor.o
	gcc -o PROCEDURA_FRYZJER PROCEDURA_FRYZJER.o operacje_semafor.o

serwer_poczekalnia: serwer_poczekalnia.o operacje_semafor.o
	gcc -o serwer_poczekalnia serwer_poczekalnia.o operacje_semafor.o

proces_klient: proces_klient.o operacje_semafor.o
	gcc -o proces_klient proces_klient.o operacje_semafor.o

proces_fryzjer:	proces_fryzjer.o operacje_semafor.o
	gcc -o proces_fryzjer proces_fryzjer.o operacje_semafor.o