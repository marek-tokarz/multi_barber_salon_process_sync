#include "header_file.h"

int main(void)
{
     // printf("[ klient %d] proces uruchomiony \n", getpid());

     int LICZBA_ZAPYTAN_KLIENTA = 1; // + jedno zapytanie do uwolnienia semafora

     // Pobierz ID procesu
     pid_t my_pid = getpid(); // do srand() i do buf.pid

     // Zainicjalizuj generator liczb losowych za pomocą ID procesu
     srand(my_pid);

     int ZAPLATA_Z_GORY_50_ZL = 1 + (rand() % 2);
     int ZAPLATA_Z_GORY_20_ZL = 1 + (rand() % 2);
     int ZAPLATA_Z_GORY_10_ZL = 1 + (rand() % 2);

     // SEMAFOR GLOBALNY DO CHRONOLOGII

     int semID; // numer semafora globalnego

     semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

     // Inicjalizacja portfela
     Banknoty portfel = {1, 2, 1}; // 100 zl

     // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW płatność z góry i potw. obsługi

     int msqid_pay;

     if ((msqid_pay = msgget(MSG_KEY_PAY, 0666 | IPC_CREAT)) == -1)
     {
          perror("msgget");
          exit(1);
     }

     // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW poczekalnia
     // zapytania do poczekalni i odpowiedzi z niej

     int msqid_wait_room; // nr kolejki komunikatów do zapytań do poczekalnia
                          // MESSAGE buf;              // komunikat do i z poczekalni
     struct msgbuf buf;

     msqid_wait_room = msgget(MSG_KEY_WAIT_ROOM, 0666); // dostęp do kolejki
     if (msqid_wait_room == -1)
     {
          perror("msgget");
          exit(1);
     }

     // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW kasa -> klient
     // reszta do wydania dla klienta przez kasę
     // (lub ewentualnie informacja, że jej nie ma - bo wtedy
     // proces klient utkwi na msgrcv)

     int msqid_change;

     if ((msqid_change = msgget(MSG_KEY_CHANGE, 0666 | IPC_CREAT)) == -1)
     {
          perror("msgget - change");
          exit(1);
     }

     // PIERWSZY KOMUNIKAT, by można było uruchomić serwer poczekalni

     buf.mtype = SERVER; // Ustawiamy 1 jako typ wiadomości - do serwera
     buf.pid = my_pid;   // treść wiadomości
     // wysłanie komunikatu
     if (msgsnd(msqid_wait_room, &buf, sizeof(buf), 0) == -1)
     {
          perror("msgsnd - do poczekalni - klient_proces");
          exit(1);
     }
     else
     {
          // printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          signalSemafor(semID, 0);
     }

     int a = 0; // do przerwania pętli cyklu: praca -> fryzjer -> praca -> praca ...

     int czas_miedzy_zapytaniami;

     while (a < LICZBA_ZAPYTAN_KLIENTA)
     {
          srand((my_pid + a));

          // należy zasymulować losowy czas między zapytaniami od poczekalni

          czas_miedzy_zapytaniami = (rand() % 10);

          // printf("czas_miedzy_zapytaniami %d\n",czas_miedzy_zapytaniami);

          // usleep(czas_miedzy_zapytaniami); // częstotliwość wysyłania komunikatów

          // SYMULACJA PRACY KLIENTA i ZARABIANIA PIENIĘDZY - LOSOWE KWOTY

          portfel.banknot50 += rand() % 3;  // Maksymalnie 2 banknoty 50 zł
          portfel.banknot20 += rand() % 6;  // Maksymalnie 5 banknotów 20 zł
          portfel.banknot10 += rand() % 11; // Maksymalnie 10 banknotów 10 zł

          // STAN PORTFELA 'po powrocie z pracy'
          /*
           printf("Zarobiono: 50 zł x %d, 20 zł x %d, 10 zł x %d\n",
                 portfel.banknot50, portfel.banknot20, portfel.banknot10);
          */

          // Wysyłanie PID do kolejki komunikatów poczekalni
          buf.mtype = SERVER; // Ustawiamy 1 jako typ wiadomości - do serwera
          buf.pid = my_pid;   // treść wiadomości
          // wysłanie komunikatu
          if (msgsnd(msqid_wait_room, &buf, sizeof(buf), 0) == -1)
          {
               perror("msgsnd - klient_proces - zapytanie do poczekalnia");
               exit(1);
          }
          else
          {
               // printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          }

          // Oczekiwanie na odpowiedź - KLIENT MUSI WIDZIEĆ CZY MA MIEJSCE W POCZEKALNI
          if (msgrcv(msqid_wait_room, &buf, sizeof(buf), my_pid, 0) == -1)
          {
               perror("msgrcv - klient_proces - zapytanie do poczekalni");
               exit(1);
          }
          else
          {
               // Działania w zależności od odpowiedzi
               if (buf.status == 1)
               {
                    // DWUSTRONNA KOLEJKA KOMUNIKATÓW

                    // FRYZJER -> KLIENT: JESTEŚ OBSŁUGIWANY

                    // KLIENT -> FRYZJER: PŁATNOŚĆ Z GÓRY

                    // KLIENT OBSŁUGIWANY PRZEZ FRYZJERA

                    struct pay obsluga;

                    // Oczekiwanie na odpowiedź - PO WEJŚCIU DO POCZEKALNI KLIENT
                    // MUSI WIEDZIEĆ CZY MA FRYZJERA DO OBSŁUGI
                    if (msgrcv(msqid_pay, &obsluga, 7 * sizeof(int), my_pid, 0) == -1)
                    {
                         perror("msgrcv - klient - potw. obsługi przez fryzjera\n");
                         exit(1);
                    }
                    else
                    {
                         // printf("[ klient %d] otrzymał potw., że będzie obsługiwany\n", getpid());
                    }
                    //
                    // KLIENT WIE, ŻE JEST OBSŁUGIWANY - ma fryzjera - WIĘC PŁACI Z GÓRY
                    struct pay Platnosc_z_gory;

                    Platnosc_z_gory.mtype = obsluga.fryzjer_PID;
                    // 160 zl

                    Platnosc_z_gory.banknoty[0] = ZAPLATA_Z_GORY_50_ZL;
                    Platnosc_z_gory.banknoty[1] = ZAPLATA_Z_GORY_20_ZL;
                    Platnosc_z_gory.banknoty[2] = ZAPLATA_Z_GORY_10_ZL;

                    if (msgsnd(msqid_pay, &Platnosc_z_gory, 7 * sizeof(int), 0) == -1)
                    {
                         perror("msgsnd - klient_proces - platnosc_z_gory\n");
                         exit(1);
                    }
                    else
                    {
                         struct change wydanie_reszty;

                         if (msgrcv(msqid_change, &wydanie_reszty, 6 * sizeof(int), my_pid, 0) == -1)
                         {
                              perror("msgrcv - klient - reszta\n");
                              exit(1);
                         }
                         else
                         {
                              int reszta_u_klienta = wydanie_reszty.banknoty[0]*50 + wydanie_reszty.banknoty[1]*20 + wydanie_reszty.banknoty[2]*10;
                              // printf("[ klient %d] otrzymał resztę: %d\n", getpid(),reszta_u_klienta);
                              //printf("[ klient ] Reszta:\n");
                              //printf("[ klient ] 50 zl *:%d\n",wydanie_reszty.banknoty[0]);
                              //printf("[ klient ] 20 zl *:%d\n",wydanie_reszty.banknoty[1]);
                              //printf("[ klient ] 10 zl *:%d\n",wydanie_reszty.banknoty[2]);
                         }    
                    }
               }

               a++; // do przerwania pętli cyklu 'życia' klienta
          }
     }

     // Obliczanie całkowitej sumy
     int suma = portfel.banknot50 * 50 + portfel.banknot20 * 20 + portfel.banknot10 * 10;
     // printf("Całkowita suma klienta PID: %d: %d zł\n", getpid(), suma);

     return 0;
}