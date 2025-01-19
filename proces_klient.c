#include "header_file.h"

int main(void)
{
     printf("proces klient: %d\n", getpid());

     // SEMAFOR GLOBALNY DO CHRONOLOGII

     int semID; // numer semafora globalnego
     int N = 5; // liczba semaforow (na razie wykoryzstywane '0' i '1')
     semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

     // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW
     // zapytania do poczekalni i odpowiedzi z niej

     int msqid; // nr kolejki komunikatów do zapytań do poczekalnia
     struct msgbuf buf; // struktura przesyłanych komunikatów
     pid_t my_pid = getpid(); // do wysłania: swój PID

     msqid = msgget(MSG_KEY, 0666); // dostęp do kolejki
     if (msqid == -1)
     {
          perror("msgget");
          exit(1);
     }

     int a = 0; // do przerwania pętli cyklu: praca -> fryzjer -> praca -> praca ...

     // PIERWSZY KOMUNIKAT, by można było uruchomić serwer poczekalni

     buf.mtype = my_pid; // Ustawiamy PID jako typ wiadomości
     buf.pid = my_pid;   // treść wiadomości
          if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1) // wysłanie komunikatu
          {
               perror("msgsnd - klient_proces");
               exit(1);
          }
          else
          {
               printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
               signalSemafor(semID, 0);
          }

     while (a < 3)
     {
          // ... kod, który wykonuje się w pętli ...
          // printf("Proces %d wykonuje swoje zadanie...\n", my_pid);

          // Wysyłanie PID do kolejki komunikatów
          buf.mtype = my_pid; // Ustawiamy PID jako typ wiadomości
          buf.pid = my_pid;   // treść wiadomości
          if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1) // wysłanie komunikatu
          {
               perror("msgsnd - klient_proces");
               exit(1);
          }
          else
          {
               printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          }

          // (po tym jak wiadomość została wysłana)
          // signalSemafor(semID, 0); // PODNIEŚ SEMAFOR 0 - żeby serwer mógł zacząć odbierać
          // do rozważenia - inny sposób zakomunikowania serwerowi, że może odbierać
          // komunikaty od klientów - PONIEWAŻ NIEPOTRZEBNIE PODNOSZĘ SEMAFOR w
          // w każdej pętli

          usleep(100); // częstotliwość wysyłania komunikatów

          a++; // do przerwania pętli cyklu 'życia' klienta
     }

     

     return 0;
}