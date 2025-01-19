#ifndef HEADER_FILE
#define HEADER_FILE

// BIBLIOTEKI

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define KEY_GLOB_SEM 1234 // klucz do globalnego semafora chronologii

#define KEY_SHM_SEM 2468 // klucz do semafora dostępu do pamięci dzielonej

#define MSG_KEY 67890  // Klucz do kolejki komunikatów - wysyłanie 
                       // zapytań przez klientów do poczekalni 
                       // i odpowiedź od poczekalni

#define SERVER 1

#define SHM_KEY 12345  // Klucz do segmentu pamięci współdzielonej
                       // miejsce na zapisywanie PIDów klientów, 
                       // ktorzy weszli do poczekalni
#define MAX_PIDS 6    // Maksymalna liczba PID-ów - pojemność poczekalni

int *pam;

#define zapis pam[MAX_PIDS + 1] // indeks zapis
#define odczyt pam[MAX_PIDS]    // indeks odczyt

/*
typedef struct {
    long    msg_to;
    long    msg_fm;
    pid_t   pid;
    int status; // 1 - sukces, 0 - brak miejsca
} MESSAGE;
*/

struct msgbuf {
    long mtype;
    pid_t pid;
    int status; // 1 - sukces, 0 - brak miejsca
};

// Struktura danych do przechowywania PID-ów
typedef struct {
    pid_t pids[MAX_PIDS]; // tablica PIDów klientów w poczekalni
    int counter; // Licznik zajętych slotów
}SharedMemory;


int alokujSemafor(key_t klucz, int number, int flagi);
void inicjalizujSemafor(int semID, int number, int val);
int waitSemafor(int semID, int number, int flags);
void signalSemafor(int semID, int number);
int zwolnijSemafor(int semID, int number);

#endif // HEADER_FILE

/*

SCHEMAT DZIAŁANIA PAMIĘCI DZIELONEJ
	
							    counter 
tablica	[0]	[1]	[2]	[3]	[4]	[5]	
    							0	counter = 0: czytający - nie może czytać
	    1						1	zapis	counter++ 
	    1	1					2	zapis	counter++
	    1	1	1				3	zapis	counter++
	    1	1	1	1			4	zapis	counter++
	    1	1	1	1	1		5	zapis	counter++
	    1	1	1	1	1	1	6	zapis   counter++; counter = 6: piszący nie może zapisać
	
	
							    counter 
tablica	[0]	[1]	[2]	[3]	[4]	[5]	
							0	counter = 0: czytający - nie może czytać
	    1						1	zapis
	    1	1					2	zapis
	    1	-					1	odczyt z tablica[counter-1]
	    1	1					2	zapis
	    1	1	1				3	zapis
	    1	1	-				2	odczyt z tablica[counter-1]

*/