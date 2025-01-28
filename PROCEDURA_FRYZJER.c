#include "header_file.h"

int main(void)
{
    printf("[PROCEDURA FRYZJER]\n");

    // KOLEJKA KOMUNIKATÓW DO OBSŁUGI SYGNAŁÓW

    int msgqid_barber;

    msgqid_barber = msgget(KEY_SIG1_BARBER, IPC_CREAT | 0666);
    if (msgqid_barber == -1)
    {
        perror("msgget msgqid_barber failed");
        exit(1);
    }

    // SEMAFOR GLOBALNY do chronologii wstępnej

    int semID;

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

    // printf("Czekam na semaforze nr 1\n");

    waitSemafor(semID, 1, SEM_UNDO); // CZEKAJ NA SEMAFORZE 1

    // SEMAFOR FOTELI

    int semID_fotel = alokujSemafor(KEY_FOTEL_SEM, 1, 0666 | IPC_CREAT);

    // Inicjalizacja semafora - licznik ustawiony na 2 (dostępne dwa zasoby)
    inicjalizujSemafor(semID_fotel, 0, LICZBA_FOTELI);

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

            MessageBarber msg;
            msg.mtype = 1;
            msg.pid = fryzjer_pid;
            if (msgsnd(msgqid_barber, &msg, sizeof(msg.pid), 0) == -1)
            {
                perror("msgsnd FRYZJER failed");
                // exit(1);
            }
            // printf("[FRYZJER] wysłał PID fryzjera %d kol. kom.\n", fryzjer_pid);
            break;
        }
    }

    // SEMAFOR DLA URUCHOMIENIA KASY
    signalSemafor(semID, 2); // PODNIEŚ SEMAFOR 2 - dla kasy

    // SEMAFOR DLA KASJERA - wysłałem PIDy fryzjerów
    signalSemafor(semID, 5); // PODNIEŚ SEMAFOR 5 - dla KASJERA

    // czekanie na wszystkie procesy potomne
    for (i = 0; i < LICZBA_FRYZJEROW; i++)
    {
        wait(NULL);
    }

    printf("Fryzjerzy zakończyli pracę\n");

    signalSemafor(semID, 3); // DO KASJERA

    return 0;
}