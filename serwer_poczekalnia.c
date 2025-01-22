#include "header_file.h"

int main()
{

    // SEMAFOR GLOBALNY

    int semID_glob; // numer semafora globalnego
    int N = 5;      // liczba semaforow (na razie wykorzystywane '0' i '1')

    semID_glob = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666); // dostęp do semafora globalnego

    int semID_shm; // numer semafora pamięci współdzielonej
    int B = 1;
    semID_shm = alokujSemafor(KEY_SHM_SEM, B, IPC_CREAT | 0666); // dostęp do semafora pamięci
    inicjalizujSemafor(semID_shm, 0, 1);

    waitSemafor(semID_glob, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0 - od klientów
    // znak, że wysyłają już komunikaty

    printf("[PROCEDURA serwer_poczekalnia]\n");

    // TWORZENIE PAMIĘCI WSPÓŁDZIELONEJ

    int shmid;         // nr pamięci współdzielonej - poczekalnia
    SharedMemory *shm; // wskaźnik na pamięć współdzieloną

    // Tworzenie segmentu pamięci współdzielonej
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | IPC_EXCL | 0666);

    // Podpięcie segmentu do przestrzeni adresowej procesu
    shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (SharedMemory *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // POCZEKALNIA PRZED PRZYBYCIEM KLIENTÓW
    /*
    waitSemafor(semID_shm, 0, SEM_UNDO);
    shm->counter = 0;
    printf("Liczba zapisanych PID-ów: %d\n", shm->counter);
    printf("Zapisane PID-y:");
    for (int i = 0; i < MAX_PIDS; i++)
    {
        printf(" %d", (int)shm->pids[i]);
    }
    printf("\n");
    signalSemafor(semID_shm, 0);
    */

    // KOLEJKA KOMUNIKATÓW klient <-> poczekalnia

    int msqid;
    struct msqid_ds buf_info; // do pozyskiwania info o ilości kom. w kolejce
    // MESSAGE buf;              // komunikat do i z poczekalni
    struct msgbuf buf;

    msqid = msgget(MSG_KEY_WAIT_ROOM, 0666); // dostęp do kolejki
    if (msqid == -1)
    {
        perror("msgget");
        exit(1);
    }

    int odebrane_komunikaty = 0;            // DO PRZERWANIA PĘTLI SERWERA
    int liczba_prob_odbioru_nieudanego = 0; // DO PRZERWANIA PĘTLI SERWERA
    // liczba prób odbioru ma zapobiec zamknięci pętli gdy akurat kolejka
    // przypadkowo była pusta, bo klienci nic nie wysłali
    int liczba_przyjętych_do_poczekalni = 0;

    int podniesiono_semafor_globalny_nr_1 = 0;
    while(podniesiono_semafor_globalny_nr_1 == 0)
    {
        if (msgctl(msqid, IPC_STAT, &buf_info) == -1) // sprawdzanie stanu kolejki
        {
            perror("msgctl");
            return 1;
        }

        if (buf_info.msg_qnum > 1) // sprawdzenie kolejki - w szczególności ilość kom.
        {
            signalSemafor(semID_glob, 1); // PODNIEŚ SEMAFOR 1 - dla fryzjerów
            podniesiono_semafor_globalny_nr_1 = 1; // ZAKOŃCZ PĘTLĘ
        }
    }

    while (liczba_prob_odbioru_nieudanego < 1000) // pętla odbioru komunikatów
    {

        // flaga IPC_NOWAIT, aby msgrcv nie zatrzymało pętli serwera
        // odbiór kom.: mtype <= -1
        if ((msgrcv(msqid, &buf, sizeof(buf), 1, IPC_NOWAIT)) == -1)
        {
            // perror("msgrcv - server");
            // zwracało - msgrcv - server: No message of desired type
            liczba_prob_odbioru_nieudanego++;
            usleep(20); // ZWOLNIJ CZEKAJĄC NA KOMUNIKAT W KOLEJCE NIM ZAKOŃCZYSZ PĘTLĘ
            // nie odebrano pożądanego komunikat, nie rób nic, kontynuuj
            // 1000 razy bez pożądanego komuniaktu i pętla się zakończy
        }
        else
        {
            odebrane_komunikaty++; // do zsumowania odebranych komunikatów
        }

        pid_t sender_pid = buf.pid; // Pobieramy PID nadawcy z mtype
        pid_t received_pid = buf.pid;

        // printf("Otrzymano PID: %d od nadawcy %d\n", received_pid, sender_pid);

        waitSemafor(semID_shm, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0

        int jest_w_poczekalni = 0;

        for (int j = 0; j < MAX_PIDS; j++)
        {
            if (shm->pids[j] == received_pid)
            {
                jest_w_poczekalni = 1;
                // printf("już jest w poczekalni\n");
            }
        }

        if (shm->counter < MAX_PIDS && jest_w_poczekalni == 0 && received_pid > 0)
        {
            shm->pids[shm->counter] = received_pid;
            shm->counter++;
            buf.mtype = sender_pid;
            buf.status = 1;
            liczba_przyjętych_do_poczekalni++;

            if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
            {
                perror("server - msgsnd - status 1");
                exit(1);
            }

            // printf("PID %d zapisany do pamięci współdzielonej.\n", received_pid);
        }
        else // brak miejsca
        {
            buf.mtype = sender_pid;
            buf.status = 0;
            if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
            {
                perror("server - msgsnd - status 0");
                exit(1);
            }
        }

        signalSemafor(semID_shm, 0);

        // na obecną chwilę poczekalnia zapisuje te same PIDy do
        // pamięci współdzielonej
        // potrzeba albo mechanizmu który to blokuje
        // albo logika procesu klienta uniemożliwi wysyłanie kolejnego komunikatu
        // jeśli już jest w kolejce w poczekalni

        usleep(80); // częstotliwość odbierania komunikatów

        /*  RACZEJ ZBĘDNĘ, PĘTLĘ KOŃCZY ZMIENNA: liczba_prob_odbioru_nieudanego
        if (msgctl(msqid, IPC_STAT, &buf_info) == -1) // sprawdzanie stanu kolejki
        {
            perror("msgctl");
            return 1;
        }

        if (buf_info.msg_qnum == 0) // sprawdzenie kolejki - w szczególności ilość kom.
        {
            // printf("serwer: Kolejka pusta.\n");
            // liczba_prob_odbioru++;
        }
        */
    }

    printf("\nLICZBA PRZYJĘTYCH DO POCZEKALNI: %d\n", liczba_przyjętych_do_poczekalni);

    waitSemafor(semID_shm, 0, SEM_UNDO);
    printf("Liczba klientów w poczekalni po zakończeniu obsługi: %d\n", shm->counter);
    printf("PID-y klientów:");
    for (int i = 0; i < shm->counter; i++)
    {
        printf(" %d", (int)shm->pids[i]);
    }
    printf("\n");
    signalSemafor(semID_shm, 0);

    msgctl(msqid, IPC_RMID, NULL); // usunięcie kolejki komunikatów
    shmctl(shmid, IPC_RMID, NULL); // usunięcie pamięci współdzielonej
    zwolnijSemafor(semID_shm, 1); // USUWANIE SEMAFOR pamieci wspóldzielonej

    printf("Odebrane komunikaty: %d\n", odebrane_komunikaty);
}