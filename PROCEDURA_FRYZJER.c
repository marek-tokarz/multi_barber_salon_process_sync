#include "header_file.h"

int main(void)
{
    printf("PROCEDURA FRYZJER\n");

    // TWORZENIE procesu: proces_fryzjer

    int fryzjer_pid;
    int liczba_fryzjerow = 3;
    int i; // do pętli tworzącej klientów

    
    for (i = 0; i < liczba_fryzjerow; i++)
    {
        fryzjer_pid = fork();
        switch (fryzjer_pid)
        {
        case -1: // Błąd przy fork
            perror("fork failed");
            exit(1);

        case 0: // Proces potomny
            // Uruchomienie nowego programu
            execl("proces_fryzjer", "proces_fryzjer", (char *)NULL);
            perror("exec: proces_klient failed"); // Jeśli execlp nie zadziałało
            exit(1);

        default: // Proces macierzysty
            break;
        }
    }

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < liczba_fryzjerow; i++)
    {
        wait(NULL);
    }

}