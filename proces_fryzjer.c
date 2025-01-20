#include "header_file.h"

int main(void)
{
    printf("proces fryzjer: %d\n", getpid());

    int semID_shm; // numer semafora pamięci współdzielonej
    int B = 1;
    semID_shm = alokujSemafor(KEY_SHM_SEM, B, IPC_CREAT | 0666); // dostęp do semafora pamięci

    int shmid;         // nr pamięci współdzielonej - poczekalnia
    SharedMemory *shm; // wskaźnik na pamięć współdzieloną

    // Tworzenie segmentu pamięci współdzielonej
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // Podpięcie segmentu do przestrzeni adresowej procesu
    shm = (SharedMemory *)shmat(shmid, NULL, 0);

    int liczba_prob_sprawdzenia_poczekalni = 0;

    int pid_klienta_do_obsługi;

    int liczba_zabranych_z_poczekalni = 0;

    while (liczba_prob_sprawdzenia_poczekalni < 10) // pętla odbioru komunikatów
    {
        waitSemafor(semID_shm, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0

        /*
        CHWILOWO FRYZJER NIE ROBI NIC - implementacja obsługi zatrzyma
        wysyłanie kolejnych zapytań o miejsce w poczekalni
        przez frzyjera
        */

        if (shm->counter > 0)
        {
            pid_klienta_do_obsługi = shm->pids[shm->counter - 1]; // zdejmij ostatniego klienta
            shm->pids[shm->counter - 1] = -1;
            shm->counter--;

            liczba_zabranych_z_poczekalni++;

            // printf("Klient o PID %d zabrany z poczekalnia.\n", pid_klienta_do_obsługi);
        }
        else // brak miejsca
        {
            // printf("Poczekalnia pusta\n");
        }

        /*
        CHWILOWO FRYZJER NIE ROBI NIC
        */

        signalSemafor(semID_shm, 0);

        liczba_prob_sprawdzenia_poczekalni++;

        usleep(200); // częstotliwość sprawdzania poczekalnia
    }


    printf("\nfryzjer PID: %d odebrał z poczekalni %d klientów\n",getpid() ,liczba_zabranych_z_poczekalni);

    return 0;
}