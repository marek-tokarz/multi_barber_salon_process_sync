#include "header_file.h"

int main(void)
{
    printf("[ fryzjer %d] proces uruchomiony\n", getpid());

    int pid_fryzjera = getpid();

    int KOSZT_OBSLUGI = 140;

    // SEMAFOR FOTELI

    int semID_fotel = alokujSemafor(KEY_FOTEL_SEM, 1, 0666 | IPC_CREAT);

    int semID_shm; // numer semafora pamięci współdzielonej
    int B = 1;
    // dostęp do semafora pamięci wspóldzielonej - poczekalnia
    semID_shm = alokujSemafor(KEY_SHM_SEM, B, IPC_CREAT | 0666);

    // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW fryzjer <-> klient
    // obsługa klienta przez fryzjera i płatność z góry przez klienta

    int msqid_pay;

    if ((msqid_pay = msgget(MSG_KEY_PAY, 0666 | IPC_CREAT)) == -1)
    {
        perror("msgget - service/paying");
        exit(1);
    }

    // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW fryjer -> kasa
    // zapłata za usługę od klienta i reszta do wydania dla klienta przez kasę

    int msqid_cash;

    if ((msqid_cash = msgget(MSG_KEY_CASH, 0666 | IPC_CREAT)) == -1)
    {
        perror("msgget - cash");
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

    while (liczba_prob_sprawdzenia_poczekalni < 100) // pętla odbioru komunikatów
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
        else
        {
            // printf("Poczekalnia pusta\n");
        }

        signalSemafor(semID_shm, 0); // sem. pamięci współdzielonej

        // OBSŁUGA KLIENTA - pobranie płatności z góry, obsłużenie
        //                   wysłanie kom. z resztą do kasy

        if (pid_klienta != 0) // na roboczo, by nie czekać na kom.
        {                     // jeśli nie mam klienta do obsługi

            struct pay obsluga;

            obsluga.mtype = pid_klienta;
            obsluga.fryzjer_PID = pid_fryzjera;

            // poinformowanie klienta odebranego z poczekalni, że będzie
            // obsługiwany przez danego fryzjera
            if (msgsnd(msqid_pay, &obsluga, 7 * sizeof(int), 0) == -1)
            {
                perror("msgsnd - fryzjera - obsługa klienta");
                exit(1);
            }
            else
            {
                // printf("[ fryzjer %d] wysłał potw., że będzie obsługiwać klienta\n", getpid());
            }

            // płatność z góry
            struct pay platnosc; // platnosc od klienta

            // printf("Czekam na płatność od klienta: %d \n",pid_klienta_do_obslugi);

            int prosby_o_platnosc_z_gory = 0;

            int dokonano_platnosci = 0;

            while (prosby_o_platnosc_z_gory < 5) // by nie czekać w nieskończoność na płatność
            {                                    // czyli nie utkwić na msgrcv

                // RACZEJ BEZ IPC_NOWAIT - fryzjer ma czekać na kasę od klienta
                if (msgrcv(msqid_pay, &platnosc, 7 * sizeof(int), pid_fryzjera, IPC_NOWAIT) == -1)
                {
                    // perror("msgrcv - fryzjer");
                    prosby_o_platnosc_z_gory++;
                    usleep(10);
                }
                else
                {
                    // printf("[ fryzjer %d] otrzymał płatność od: %d\n", getpid(), pid_klienta);
                    prosby_o_platnosc_z_gory = 6; // ZAKOŃCZ PĘTLĘ czekania na płatność
                    dokonano_platnosci = 1;
                }

                usleep(50);
            }

            

            prosby_o_platnosc_z_gory = 0; // by w kolejnych programach czekać na płatność

            if (dokonano_platnosci == 0)
            {
                printf("[ fryzjer ] klient nie zapłacił\n");
            }
            else
            {
                // FRYZJER OTRZYMAŁ KWOTĘ/BANKNOTY:
                /*
                printf("[fryzjer] otrzymał kwotę:\n");
                printf("Płatność - banknoty 50 zł: %d\n",platnosc.banknoty[0]);
                printf("Płatność - banknoty 20 zł: %d\n",platnosc.banknoty[1]);
                printf("Płatność - banknoty 10 zł: %d\n",platnosc.banknoty[2]);
                */
                // SEMAFOR FOTELA - semafor liczący
                if (waitSemafor(semID_fotel, 0, 0) == 1)
                {
                    // printf("[ fryzjer %d] mam fotel\n", pid_fryzjera);
                    usleep(10); // Symulacja użycia zasobu - fotela - strzyżenie
                    // printf("[ fryzjer %d] zwalniam fotel\n", pid_fryzjera);
                    signalSemafor(semID_fotel, 0);
                }

                // WYSYŁANIE PIENIĘDZY OD KLIENTA DO KASY
                struct cash zaplata_reszta;

                int reszta_do_wydania = 0;
                int wplata_od_klienta = platnosc.banknoty[0]*50 + platnosc.banknoty[1]*20 + platnosc.banknoty[2]*10;
                reszta_do_wydania = wplata_od_klienta - KOSZT_OBSLUGI;  

                // printf("Reszta wyliczona u fryzjera: %d\n",reszta_do_wydania);

                zaplata_reszta.mtype = CASH_REGISTER;
                zaplata_reszta.klient_PID = pid_klienta;
                zaplata_reszta.reszta_dla_klienta = reszta_do_wydania;
                zaplata_reszta.wplata_klienta = wplata_od_klienta;
                zaplata_reszta.banknoty[0] = platnosc.banknoty[0];
                zaplata_reszta.banknoty[1] = platnosc.banknoty[1];
                zaplata_reszta.banknoty[2] = platnosc.banknoty[2];

                /*
                printf("zaplata_reszta.mtype %ld\n",zaplata_reszta.mtype);
                printf("zaplata_reszta.klient_PID %d\n",zaplata_reszta.klient_PID );
                printf("zaplata_reszta.reszta_dla_klienta %d\n",zaplata_reszta.reszta_dla_klienta);
                printf("zaplata_reszta.banknoty[0] %d\n",zaplata_reszta.banknoty[0] );
                printf("zaplata_reszta.banknoty[1] %d\n",zaplata_reszta.banknoty[1]);
                printf("zaplata_reszta.banknoty[2] %d\n",zaplata_reszta.banknoty[2]);
                */

                if (msgsnd(msqid_cash, &zaplata_reszta, 6 * sizeof(int), 0) == -1)
                {
                    perror("msgsnd - fryzjera - zaplata_reszta");
                    exit(1);
                }
                else
                {
                    // printf("[ fryzjer %d] wysłał zapłatę klienta\n", getpid());
                }
            }
        }

        pid_klienta = 0;

        liczba_prob_sprawdzenia_poczekalni++;

        usleep(200); // częstotliwość sprawdzania poczekalnia
    }

    shmdt(shm); // Odłączamy pamięć współdzieloną

    printf("\n[ fryzjer %d ]: odebrał z poczekalni %d klientów\n", getpid(), liczba_zabranych_z_poczekalni);

    return 0;
}