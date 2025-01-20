#include "header_file.h"

int main(void)
{
    printf("proces fryzjer: %d\n", getpid());

    int semID_shm; // numer semafora pamięci współdzielonej
    int B = 1;
    // dostęp do semafora pamięci wspóldzielonej - poczekalnia
    semID_shm = alokujSemafor(KEY_SHM_SEM, B, IPC_CREAT | 0666);

    // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW płatność z góry

    int msqid_pay;

    if ((msqid_pay = msgget(MSG_KEY_PAY, 0666 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }

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

    int pid_klienta = 0;

    int liczba_zabranych_z_poczekalni = 0;

    while (liczba_prob_sprawdzenia_poczekalni < 10) // pętla odbioru komunikatów
    {
        waitSemafor(semID_shm, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0

        if (shm->counter > 0)
        {
            // SPRAWIEDLIWA KOLEJKA W POCZEKALNI:

            // Zabierz pierwszego klienta z poczekalni i przesuń pozostałych
            pid_klienta = shm->pids[0];

            // Przesuń pozostałe PID-y o jedno miejsce do przodu
            for (int i = 0; i < shm->counter - 1; i++)
            {
                shm->pids[i] = shm->pids[i + 1];
            }

            shm->counter--; // dekrementacja - zabrano pierwszego klienta

            liczba_zabranych_z_poczekalni++;

            // printf("Klient o PID %d zabrany z poczekalnia.\n", pid_klienta_do_obslugi);
        }
        else // brak miejsca
        {
            // printf("Poczekalnia pusta\n");
        }

        signalSemafor(semID_shm, 0); // sem. pamięci współdzielonej

        // OBSŁUGA KLIENTA - pobranie płatności z góry, obsłużenie
        //                   wysłanie kom. z resztą do kasy

        if (pid_klienta != 0) // na roboczo, by nie czekać na kom.
        {                     // jeśli nie mam klienta do obsługi
            // płatność z góry

            struct pay platnosc; // platnosc od klienta

            // printf("Czekam na płatność od klienta: %d \n",pid_klienta_do_obslugi);

            int prosby_o_platnosc_z_gory = 0;

            while (prosby_o_platnosc_z_gory < 5) // by nie czekać w nieskończoność na płatność
            {                                    // czyli nie utkwić na msgrcv
                 
                if (msgrcv(msqid_pay, &platnosc, sizeof(platnosc.kwota), pid_klienta, IPC_NOWAIT) == -1)
                {
                    // perror("msgrcv - fryzjer");
                    prosby_o_platnosc_z_gory++;
                    usleep(10);
                }
                else
                {
                    printf("Fryzjer otrzymał płatność od: %d\n", pid_klienta);
                    prosby_o_platnosc_z_gory = 3; // ZAKOŃCZ PĘTLĘ czekania na płatność
                }

                prosby_o_platnosc_z_gory++;

                usleep(100);
            }

            if (prosby_o_platnosc_z_gory != 0)
            {
                printf("Klient nie zapłacił\n");
            }

            // COŚ POWODUJE BŁĄD: *** stack smashing detected ***: terminated
        }

        pid_klienta = 0;

        liczba_prob_sprawdzenia_poczekalni++;

        usleep(200); // częstotliwość sprawdzania poczekalnia
    }

    shmdt(shm); // Odłączamy pamięć współdzieloną

    printf("\nfryzjer PID: %d odebrał z poczekalni %d klientów\n", getpid(), liczba_zabranych_z_poczekalni);

    return 0;
}