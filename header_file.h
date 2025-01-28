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
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

// ZMIENNE DLA PROCESÓW

int LICZBA_FOTELI = 6;

int LICZBA_FRYZJEROW = 8;

#define MAX_PIDS 10    // Maksymalna liczba PID-ów - pojemność poczekalni
  
int LICZBA_KLIENTOW = 30;

// # # # SEMAFORY

int N = 6; // LICZBA SEMAFORÓW GLOBALNYCH do wstępnej chronologii uruchomienia

#define KEY_SEM_KASA 82641 // klucz do semafora kasy - wpłaty z góry i wypłaty reszty

#define KEY_GLOB_SEM 1234 // klucz do globalnego semafora chronologii

#define KEY_SHM_SEM 2468 // klucz do semafora dostępu do pamięci dzielonej

#define KEY_FOTEL_SEM 3456 // klucz do semafora fotel

// # # # KOLEJKI KOMUNIKATÓW DO WYSYŁANIA SYGNAŁÓW

#define KEY_SIG1_BARBER 9283 // klucz do globalnego semafora chronologii

typedef struct {
    long mtype;
    pid_t pid;
} MessageBarber;

// # # # KOLEJKI KOMUNIKATÓW DO OBSŁUGI SALONU

// KOLEJKA DWUSTRONNA
#define MSG_KEY_WAIT_ROOM 67890  // Klucz do kolejki komunikatów - wysyłanie 
                       // zapytań przez klientów do poczekalni 
                       // i odpowiedź od poczekalni
#define SERVER 1

// Struktura komunikatu w kolejce dwukierunkowej: klient <-> poczekalnia
struct msgbuf {
    long mtype;
    pid_t pid;
    int status; // 1 - sukces, 0 - brak miejsca
};

// KOLEJKA DWUSTRONNA
#define MSG_KEY_PAY 9753 // klucz kolejki komunikatów do płacenia z góry przez klienta
						 // oraz do informowania klienta, że będie obsługiwany

// Struktura komunikatu do płacenia z góry przez klienta
// oraz do informowania klienta, że będzie obsługiwny
struct pay {
    long mtype;       // Typ komunikatu
	int klient_PID;
	int fryzjer_PID;
	int banknoty[3];
	int kwota_do_zaplaty;
	int reszta;
	// SUMA BAJTÓW: 28 (7 x int)
};

// KOLEJKA JEDNOSTRONNA
#define MSG_KEY_CASH 7531	// Klucz do kolejki komunikatów od fryzjera do kasy
						// czyli do przekazania opłaty klienta i poinformowania 
						// o reszczie dla niego

#define CASH_REGISTER 1

struct cash {
    long mtype;       // Typ komunikatu
	int klient_PID;
	int reszta_dla_klienta;
	int wplata_klienta;
	int banknoty[3];
	// SUMA BAJTÓW: 24 (6 x int)
};

// KOLEJKA JEDNOSTRONNA
#define MSG_KEY_CHANGE 5382	// Klucz do kolejki komunikatów od kasy do klienta
						    // czyli wydanie reszty z kasy
struct change { 		
    long mtype;       // Typ komunikatu
	int klient_PID;
	int reszta;
	int banknoty[3];
	// SUMA BAJTÓW: 20 (5 x int)
};
// struktura analogiczna i podobna jak wyżej, ale dla rozróżnienia i czytelności kodu
// wprowadzona ponownie pod inną nazwą


// # # # PAMIĘĆ DZIELONA

#define SHM_KEY 12345  // Klucz do segmentu pamięci współdzielonej
                       // miejsce na zapisywanie PIDów klientów, 
                       // ktorzy weszli do poczekalni
// SCHEMAT DZIAŁANIA PAMIĘCI NA SAMYM DOLE, POD '#endif // HEADER_FILE'

// Struktura danych do przechowywania PID-ów
typedef struct {
    pid_t pids[MAX_PIDS]; // tablica PIDów klientów w poczekalni
    int counter; // Licznik osób w poczekalni
}SharedMemory;


/* NIE DZIAŁA
#define LICZBA_KLIENTOW 10
#define LICZBA_ZAPYTAN_KLIENTA 9 // + jedno zapytanie do uwolnienia semafora
#define LICZBA_FRYZJEROW 10
*/

// Struktura danych - 'portfel' klienta i kasa
typedef struct {
    int banknot50;
    int banknot20;
    int banknot10;
} Banknoty;

// DEKLARACJE FUNKCJI SEMAFORÓW

int alokujSemafor(key_t klucz, int number, int flagi);
void inicjalizujSemafor(int semID, int number, int val);
int waitSemafor(int semID, int number, int flags);
void signalSemafor(int semID, int number);
int zwolnijSemafor(int semID, int number);

#endif // HEADER_FILE

/*
SCHEMAT DZIAŁANIA PAMIĘCI DZIELONEJ

'SPRAWIEDLIWA KOLEJKA'
-- zmiana w pamięci: fryzjer zabiera klienta tablica[0] - pierwszy w poczekalni (counter = 1)
	
SCHEMAT 6 SEKWENCYJNYCH ZAPISÓW DO POCZEKALNI:

							    counter 
tablica	[0]	[1]	[2]	[3]	[4]	[5]	
    							0	counter = 0: czytający (fryzjer) - nie może czytać (brak klientów)
	    1						1	zapis	counter++ 
	    1	1					2	zapis	counter++
	    1	1	1				3	zapis	counter++
	    1	1	1	1			4	zapis	counter++
	    1	1	1	1	1		5	zapis	counter++
	    1	1	1	1	1	1	6	zapis   counter++; counter = 6: piszący 
											(poczekalnia) nie może zapisać - brak miejsc w poczekalni

SCHEMAT 4 ZAPISÓW  i 2 ODCZYTÓW Z POCZEKALNI:

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