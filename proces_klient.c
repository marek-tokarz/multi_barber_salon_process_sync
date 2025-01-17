#include "header_file.h"

int main(void)
{
     printf("proces klient: %d\n", getpid());

     int LICZBA_ZAPYTAN_KLIENTA = 10;

     // SEMAFOR GLOBALNY DO CHRONOLOGII

     int semID; // numer semafora globalnego
     int N = 5; // liczba semaforow (na razie wykoryzstywane '0' i '1')
     semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

     // Inicjalizacja portfela
     Banknoty portfel = {1, 2, 1}; // 100 zl

     // Pobierz ID procesu
     pid_t my_pid = getpid(); // do srand() i do buf.pid

     // Zainicjalizuj generator liczb losowych za pomocą ID procesu
     srand(my_pid);

     // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW płatność z góry

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

     // PIERWSZY KOMUNIKAT, by można było uruchomić serwer poczekalni

     buf.mtype = SERVER; // Ustawiamy 1 jako typ wiadomości - do serwera
     buf.pid = my_pid;   // treść wiadomości
     // wysłanie komunikatu
     if (msgsnd(msqid_wait_room, &buf, sizeof(buf), 0) == -1)
     {
          perror("msgsnd - klient_proces");
          exit(1);
     }
     else
     {
          // printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          signalSemafor(semID, 0);
     }

     int a = 0; // do przerwania pętli cyklu: praca -> fryzjer -> praca -> praca ...

     while (a < LICZBA_ZAPYTAN_KLIENTA)
     {
          // SYMULACJA PRACY KLIENTA i ZARABIANIA PIENIĘDZY - LOSOWE KWOTY

          srand((my_pid + a));

          portfel.banknot50 += rand() % 3;  // Maksymalnie 2 banknoty 50 zł
          portfel.banknot20 += rand() % 6;  // Maksymalnie 5 banknotów 20 zł
          portfel.banknot10 += rand() % 11; // Maksymalnie 10 banknotów 10 zł

          // STAN PORTFELA 'po powrocie z pracy'
          /*
           printf("Zarobiono: 50 zł x %d, 20 zł x %d, 10 zł x %d\n",
                 portfel.banknot50, portfel.banknot20, portfel.banknot10);
          */

          // Wysyłanie PID do kolejki komunikatów
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

          usleep(100); // częstotliwość wysyłania komunikatów

          // Oczekiwanie na odpowiedź
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
                    // KLIENT OBSŁUGIWANY PRZEZ FRYZJERA

                    struct pay Platnosc_z_gory;

                    /*
                    Platnosc_z_gory.mtype = my_pid;
                    // 160 zl
                    Platnosc_z_gory.banknot50 = 2;
                    Platnosc_z_gory.banknot50 = 2;
                    Platnosc_z_gory.banknot50 = 2;
                    */

                    Platnosc_z_gory.mtype = my_pid;
                    // 160 zl

                    Platnosc_z_gory.kwota[0] = 2;
                    Platnosc_z_gory.kwota[1] = 2;
                    Platnosc_z_gory.kwota[2] = 2;

                    if (msgsnd(msqid_pay, &Platnosc_z_gory, sizeof(Platnosc_z_gory.kwota), 0) == -1)
                    {
                         perror("msgsnd - klient_proces - platnosc_z_gory");
                         exit(1);
                    }
                    else
                    {
                         printf("Klient %d wysłał płatność z góry.\n", my_pid);
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