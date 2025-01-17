#include "header_file.h"

int main(void)
{
    printf("PROCEDURA FRYZJER\n");

    int LICZBA_FRYZJEROW = 4;

    // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW płatność z góry
    // TYLKO DO CELU JEJ USUNIĘCIA, GDY ZNIKNĄ fryzjerzy
     int msqid_pay;

     if ((msqid_pay = msgget(MSG_KEY_PAY, 0666 | IPC_CREAT)) == -1)
     {
          perror("msgget");
          exit(1);
     }


    int semID;
    int N = 5;

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

    waitSemafor(semID, 1, SEM_UNDO); // CZEKAJ NA SEMAFORZE 1

    // TWORZENIE procesu: proces_fryzjer

    int fryzjer_pid;
    int i; // do pętli tworzącej klientów

    for (i = 0; i < LICZBA_FRYZJEROW; i++)
    {
        fryzjer_pid = fork();
        switch (fryzjer_pid)
        {
        case -1: // Błąd przy fork
            perror("fork failed");
            exit(1);
        case 0: // Proces potomny - uruchomienie nowego programu
            execl("proces_fryzjer", "proces_fryzjer", (char *)NULL);
            perror("exec: proces_klient failed"); // Jeśli execlp nie zadziałało
            exit(1);
        default: // Proces macierzysty
            break;
        }
    }

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < LICZBA_FRYZJEROW; i++)
    {
        wait(NULL);
    }

    printf("Fryzjerzy zakończyli pracę\n");

    // Usuwanie kolejki po zniknięciu fryzjerów
    msgctl(msqid_pay, IPC_RMID, NULL); 
    return 0;
}