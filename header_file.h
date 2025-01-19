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

#define MSG_KEY 67890  // Klucz do kolejki komunikatów - wysyłanie 
                       // zapytań przez klientów do poczekalni 
                       // i odpowiedź od poczekalni

// Struktura wiadomości - zapytania o miejsce w poczekalni
struct msgbuf {
    long mtype;
    pid_t pid;
    int status; // 1 - sukces, 0 - brak miejsca
};


int alokujSemafor(key_t klucz, int number, int flagi);
void inicjalizujSemafor(int semID, int number, int val);
int waitSemafor(int semID, int number, int flags);
void signalSemafor(int semID, int number);
int zwolnijSemafor(int semID, int number);

#endif // HEADER_FILE