#include "header_file.h"

// do zakończenia głównej pętli sygnałem

volatile sig_atomic_t keep_running = 1;

void handle_sigusr1(int signum)
{
    if (signum == SIGUSR1)
    {
        keep_running = 0;
        printf("[ poczekalnia ]: Otrzymano sygnał SIGUSR1 - KONIEC PĘTLI\n");
    }
}

// DO EWAKUACJI KLIENTÓW

volatile sig_atomic_t sigusr2_received = 0;

int semID_shm; // numer semafora pamięci współdzielonej

SharedMemory *shm;

void handle_sigusr2(int signum)
{
    if (signum == SIGUSR2)
    {
        sigusr2_received = 1;
        printf("[ poczekalnia ]: Otrzymano sygnał SIGUSR2 \n");
    }
}

int main()
{
    int OBIEGI_PĘTLI_SPRAWDZANIA = 10000000;

    // blokowanie sygnałów, by odebrać je w pożądanym momencie:

    sigset_t block_mask, old_mask;

    // Utwórz maskę i dodaj SIGUSR1 i SIGUSR2
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGUSR2);

    // Zablokuj sygnały od samego początku, zachowując poprzednią maskę
    if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1)
    {
        perror("sigprocmask (blocking)");
        return 1;
    }

    // ODBIÓR SIGUSR1 i/lub SIGUSR2

    // SIGUSR1
    struct sigaction sa_sigusr1;
    sa_sigusr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_sigusr1.sa_mask);
    sa_sigusr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_sigusr1, NULL);

    // SIGUSR2
    struct sigaction sa_sigusr2;
    sa_sigusr2.sa_handler = handle_sigusr2;
    sigemptyset(&sa_sigusr2.sa_mask);
    sa_sigusr2.sa_flags = 0;
    sigaction(SIGUSR2, &sa_sigusr2, NULL);

    // SEMAFOR GLOBALNY

    int semID_glob; // numer semafora globalnego

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
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);

    // Podpięcie segmentu do przestrzeni adresowej procesu
    shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (SharedMemory *)-1)
    {
        perror("poczekalnia - shmat");
        exit(1);
    }

    // POCZEKALNIA PRZED PRZYBYCIEM KLIENTÓW

    waitSemafor(semID_shm, 0, SEM_UNDO);
    shm->counter = 0;
    printf("[ poczekalnia ] przed obsługą: Liczba zapisanych PID-ów: %d\n", shm->counter);
    printf("[ poczekalnia ] Zapisane PID-y:");
    for (int i = 0; i < MAX_PIDS; i++)
    {
        printf(" %d", (int)shm->pids[i]);
    }
    printf("\n");
    signalSemafor(semID_shm, 0);

    // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW
    // KOLEJKA KOMUNIKATÓW: klient <-> poczekalnia

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

    int odebrane_komunikaty = 0; // DO PODLICZENIA ODEBRANYCH KOMUNIKATÓW
    int liczba_prob_odbioru = 0; // DO PRZERWANIA PĘTLI SERWERA - OPCJONALNIE z SIGUSR1

    int liczba_przyjętych_do_poczekalni = 0;

    int podniesiono_semafor_globalny_nr_1 = 0;
    while (podniesiono_semafor_globalny_nr_1 == 0)
    {
        if (msgctl(msqid, IPC_STAT, &buf_info) == -1) // sprawdzanie stanu kolejki
        {
            perror("msgctl");
            return 1;
        }

        if (buf_info.msg_qnum > 1) // sprawdzenie kolejki - w szczególności ilość kom.
        {
            signalSemafor(semID_glob, 1);          // PODNIEŚ SEMAFOR 1 - dla fryzjerów
            podniesiono_semafor_globalny_nr_1 = 1; // ZAKOŃCZ PĘTLĘ
        }
    }

    while (/*liczba_prob_odbioru < OBIEGI_PĘTLI_SPRAWDZANIA &&*/ keep_running == 1) // pętla odbioru komunikatów
    {

        // flaga IPC_NOWAIT, aby msgrcv nie zatrzymało pętli serwera
        
        if ((msgrcv(msqid, &buf, sizeof(buf), 1, IPC_NOWAIT)) == -1)
        {
            // perror("msgrcv - server");
            // zwracało - msgrcv - server: No message of desired type
        }
        else
        {
            odebrane_komunikaty++; // do zsumowania odebranych komunikatów
            // printf("[ poczekalnia ] odebrane_komunikaty: %d\n",odebrane_komunikaty);
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

            if (msgsnd(msqid, &buf, sizeof(buf), /*0*/ IPC_NOWAIT) == -1)
            {
                // perror("poczekalnia - msgsnd - status 1");
                // exit(1);
            }

            // printf("PID %d zapisany do pamięci współdzielonej.\n", received_pid);
        }
        else // brak miejsca
        {
            buf.mtype = sender_pid;
            buf.status = 0;
            if (msgsnd(msqid, &buf, sizeof(buf), /*0*/ IPC_NOWAIT) == -1)
            {
                // perror("poczekalnia - msgsnd - status 0");
                // exit(1);
            }
        }

        signalSemafor(semID_shm, 0);

        // Odblokuj sygnały SIGUSR1 i SIGUSR2, używając starej maski
        if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1)
        {
            perror("sigprocmask (unblocking)");
            return 1;
        }

        // MOMENT NA ODEBRANIE SYGNAŁÓW

        // PONOWNIE ZABLOKUJ SYGNAŁY

        // Utwórz maskę i dodaj SIGUSR1 i SIGUSR2
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGUSR1);
        sigaddset(&block_mask, SIGUSR2);

        // Zablokuj sygnały
        if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1)
        {
            perror("sigprocmask (blocking)");
            return 1;
        }

        if (sigusr2_received == 1)
        {
            printf("[ poczekalnia ] Ewakuacja klientów!\n");

            int PID;

            int licznik = shm->counter;

            printf("[ poczekalnia ] ilość ewakuowanych: %d\n",licznik);

            for (int i = 0; i < licznik; i++)
            {
                PID = shm->pids[i];
                printf("%d [ poczekalnia ]: usuwam PID %d\n", i ,PID);
                shm->pids[i] = 0;
            }

            shm->counter = 0;
        }

        sigusr2_received = 0;

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

        liczba_prob_odbioru++;
    }

    printf("\n[ poczekalnia ] liczba przyjętych : %d\n", liczba_przyjętych_do_poczekalni);

    waitSemafor(semID_shm, 0, SEM_UNDO);
    printf("[ poczekalnia ] liczba klientów w poczekalni po zakończeniu obsługi: %d\n", shm->counter);
    printf("[ poczekalnia ] PID-y klientów w poczekalni po zakończeniu obsługi");
    for (int i = 0; i < shm->counter; i++)
    {
        printf(" %d", (int)shm->pids[i]);
    }
    printf("\n");
    signalSemafor(semID_shm, 0);

    msgctl(msqid, IPC_RMID, NULL); // usunięcie kolejki komunikatów
    shmctl(shmid, IPC_RMID, NULL); // usunięcie pamięci współdzielonej
  
    printf("[ poczekalnia ] Odebrane komunikaty łącznie: %d\n", odebrane_komunikaty);
}