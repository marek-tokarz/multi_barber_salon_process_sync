#include "header_file.h"

int main()
{
    int semID; // numer semafora globalnego
    int N = 5; // liczba semaforow (na razie wykoryzstywane '0' i '1')

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666); // dostęp do smeafora

    waitSemafor(semID, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0 - od klientów
    // znak, że wysyłają już komunikaty

    printf("PROCEDURA serwer_poczekalnia\n");

    int msqid;
    struct msqid_ds buf_info; // do pozyskiwania info o ilości kom. w kolejce
    struct msgbuf buf;        // komunikat do i z poczekalni

    msqid = msgget(MSG_KEY, 0666); // dostęp do kolejki
    if (msqid == -1)
    {
        perror("msgget");
        exit(1);
    }

    int odebrane_komunikaty = 0; // DO PRZERWANIA PĘTLI SERWERA
    int liczba_prob_odbioru = 0; // DO PRZERWANIA PĘTLI SERWERA
    // liczba prób odbioru ma zapobiec zamknięci pętli gdy akurat kolejka
    // przypadkowo była pusta, bo klienci nic nie wysłali

    if (msgctl(msqid, IPC_STAT, &buf_info) == -1) // sprawdzanie stanu kolejki
    {
        perror("msgctl");
        return 1;
    }

    if (buf_info.msg_qnum > 1) // sprawdzenie kolejki - w szczególności ilość kom.
    {
        signalSemafor(semID, 1); // PODNIEŚ SEMAFOR 1 - dla fryzjerów
    }

    while (liczba_prob_odbioru < 1000) // pętla odbioru komunikatów
    {

        // flaga IPC_NOWAIT, aby msgrcv nie zatrzymało pętli serwera
        if ((msgrcv(msqid, &buf, sizeof(buf), 0, IPC_NOWAIT)) == -1) // odbiór dowolnego kom.
        { /* odebrano nieprawidłowo komunikat, nie rób nic, kontynuuj 
             obsługa pustej kolejki zaimplementowana dalej
             1000 razy pusta kolejka przerwie pętlę
          */ 
        } 
        else
        {
            odebrane_komunikaty++; // do zsumowania odebranych komunikatów
        }

        usleep(80); // częstotliwość odbierania komunikatów

        if (msgctl(msqid, IPC_STAT, &buf_info) == -1) // sprawdzanie stanu kolejki
        {
            perror("msgctl");
            return 1;
        }

        if (buf_info.msg_qnum == 0) // sprawdzenie kolejki - w szczególności ilość kom.
        {
            // printf("serwer: Kolejka pusta.\n");
            liczba_prob_odbioru++;
        }
    }

    msgctl(msqid, IPC_RMID, NULL); // usunięcie kolejki komunikatów

    printf("Odebrane komunikaty: %d\n", odebrane_komunikaty);
}