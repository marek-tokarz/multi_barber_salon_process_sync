#include "header_file.h"

int main(void)
{
    printf("PROCEDURA KLIENT\n");

    int semID;
    int N = 5;

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

    // TWORZENIE procesów: proces_klient

    int klient_pid;
    int liczba_klientow = 3;
    int i; // do pętli tworzącej klientów
    
    for (i = 0; i < liczba_klientow; i++)
    {
        klient_pid = fork();
        switch (klient_pid)
        {
        case -1: // Błąd przy fork
            perror("fork failed");
            exit(1);
        case 0: // Proces potomny - uruchomienie nowego programu
            execl("proces_klient", "proces_klient", (char *)NULL);
            perror("exec: proces_klient failed"); // Jeśli execlp nie zadziałało
            exit(1);
        default: // Proces macierzysty
            break;
        }
    }

    signalSemafor(semID, 0); // PODNIEŚ SEMAFOR 0

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < liczba_klientow; i++)
    {
        wait(NULL);
    }
}