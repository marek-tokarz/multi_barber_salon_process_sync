#include "header_file.h"

int main(void)
{
    printf("PROCEDURA KLIENT\n");

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

        case 0: // Proces potomny
            // Uruchomienie nowego programu
            execl("proces_klient", "proces_klient", (char *)NULL);
            perror("exec: proces_klient failed"); // Jeśli execlp nie zadziałało
            exit(1);

        default: // Proces macierzysty
            break;
        }
    }

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < liczba_klientow; i++)
    {
        wait(NULL);
    }
}