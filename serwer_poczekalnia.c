#include "header_file.h"

int main()
{
    int semID; // numer semafora globalnego
    int N = 5; // liczba semaforow (na razie wykoryzstywane '0' i '1')

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666); // dostęp do smeafora
    SharedMemory *shm;                                        // wskaźnik na pamięć współdzieloną

    waitSemafor(semID, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0 - od klientów
    // znak, że wysyłają już komunikaty

    printf("PROCEDURA serwer_poczekalnia\n");

    // TWORZENIE PAMIĘCI WSPÓŁDZIELONEJ

    int shmid; // nr pamięci współdzielonej - poczekalnia

    // Tworzenie segmentu pamięci współdzielonej
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // Podpięcie segmentu do przestrzeni adresowej procesu
    shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (SharedMemory *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Inicjalizacja pamięci współdzielonej (tylko w pierwszym procesie)

    shm->count = 0;
    memset(shm->pids, 0, sizeof(shm->pids));

    int msqid;
    struct msqid_ds buf_info; // do pozyskiwania info o ilości kom. w kolejce
    // MESSAGE buf;              // komunikat do i z poczekalni
    struct msgbuf buf;

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
        // odbiór kom.: mtype <= -1
        if ((msgrcv(msqid, &buf, sizeof(buf), 1, IPC_NOWAIT)) == -1)
        {
            // perror("msgrcv - server"); 
            // zwracało - msgrcv - server: No message of desired type 
            liczba_prob_odbioru++;
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

        if (shm->count < MAX_PIDS)
        {
            shm->pids[shm->count] = received_pid;
            shm->count++;
            buf.mtype = sender_pid;
            buf.status = 1;

            if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
            {
                perror("server - msgsnd - status 1");
                exit(1);
            }

            printf("PID %d zapisany do pamięci współdzielonej.\n", received_pid);
        }
        else // brak miejsca
        {
            buf.mtype = sender_pid;
            buf.status = 1;
            if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
            {
                perror("server - msgsnd - status 0");
                exit(1);
            }
        }

        // na obecną chwilę poczekalnia zapisuje te same PIDy do
        // pamięci współdzielonej
        // potrzeba albo mechanizmu który to blokuje
        // albo logika procesu klienta uniemożliwi wysyłanie kolejnego komunikatu
        // jeśli już jest w kolejce w poczekalni

        usleep(80); // częstotliwość odbierania komunikatów

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
    }

    printf("\nZawartość pamięci współdzielonej:\n");
    printf("Liczba zapisanych PID-ów: %d\n", shm->count);
    printf("Zapisane PID-y:");
    for (int i = 0; i < shm->count; i++)
    {
        printf(" %d", (int)shm->pids[i]);
    }
    printf("\n");

    msgctl(msqid, IPC_RMID, NULL); // usunięcie kolejki komunikatów
    shmctl(shmid, IPC_RMID, NULL); // usunięcie pamięci współdzielonej

    printf("Odebrane komunikaty: %d\n", odebrane_komunikaty);
}