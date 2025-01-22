#include "header_file.h"

int main(void)
{
    printf("[PROCEDURA KLIENT]\n");

    int LICZBA_KLIENTOW = 10;

    int semID; // numer semafora globalnego
    int N = 5; // liczba semaforow (na razie wykoryzstywane '0' i '1') // dostęp do smeafora

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

    // TWORZENIE kolejki komunikatów do zapytań i 
    // odpowiedzi z poczekalni
    int msqid; // nr kolejki komunikatów do zapytań do poczekalnia
 

    // Tworzenie kolejki komunikatów
    msqid = msgget(MSG_KEY_WAIT_ROOM, IPC_CREAT | 0666);
    if (msqid == -1)
    {
        perror("msgget");
        exit(1);
    }

    // TWORZENIE procesów: proces_klient

    int klient_pid;
    int i; // do pętli tworzącej klientów
    
    for (i = 0; i < LICZBA_KLIENTOW; i++)
    {
        klient_pid = fork();
        switch (klient_pid)
        {
        case -1: // Błąd przy fork
            perror("fork failed");
            exit(1);
        case 0: // Proces potomny - uruchomienie nowego programu
            execl("proces_klient", "proces_klient", (char *)NULL);
            perror("exec: proces_klient failed"); 
            exit(1); // Jeśli execlp nie zadziałało
        default: // Proces macierzysty
            break;
        }
    }

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < LICZBA_KLIENTOW; i++)
    {
        wait(NULL);
    }

}

/*

PRZEPEŁNIENIE KOLEJKI KOMUNIKATÓW

------ Kolejki komunikatów ---
klucz      id_msq     właściciel uprawn.    bajtów      komunikatów
0x00010932 0          marek-toka 666        16384        1024        

//  16384 bajtów podzielone przez 1024 równa się 16.

// 9 KOMUNIKATÓW:

marek-tokarz:~$ ipcs

------ Kolejki komunikatów ---
klucz      id_msq     właściciel uprawn.    bajtów      komunikatów
0x00010932 4          marek-toka 666        144          9           

*/