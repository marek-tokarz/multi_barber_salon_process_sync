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
                // MESSAGE buf;              // komunikat do i z poczekalni
     struct msgbuf buf;
     pid_t my_pid = getpid(); // do wysłania: swój PID

     msqid = msgget(MSG_KEY, 0666); // dostęp do kolejki
     if (msqid == -1)
     {
          perror("msgget");
          exit(1);
     }

     // PIERWSZY KOMUNIKAT, by można było uruchomić serwer poczekalni

     buf.mtype = SERVER; // Ustawiamy 1 jako typ wiadomości - do serwera
     buf.pid = my_pid; // treść wiadomości
     // wysłanie komunikatu
     if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
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

     while (a < 9)
     {
          // ... kod, który wykonuje się w pętli ...
          // printf("Proces %d wykonuje swoje zadanie...\n", my_pid);

          // Wysyłanie PID do kolejki komunikatów
          buf.mtype = SERVER; // Ustawiamy 1 jako typ wiadomości - do serwera
          buf.pid = my_pid; // treść wiadomości
          // wysłanie komunikatu
          if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
          {
               perror("msgsnd - klient_proces");
               exit(1);
          }
          else
          {
               // printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          }

          usleep(100); // częstotliwość wysyłania komunikatów

          // Oczekiwanie na odpowiedź
          if (msgrcv(msqid, &buf, sizeof(buf), my_pid, 0) == -1)
          {
               perror("msgrcv - klient_proces");
               exit(1);
          }
          else
          {
               // printf("Odpowiedź od poczekalni - brak miejsca, PID:%d\n",getpid());
               // Działania w zależności od odpowiedzi
               if (buf.status == 1)
               {
                    // printf("Klient %d - potw. dod. do poczekalni.\n", my_pid);
                    // ... Działania po udanym zapisaniu PID do pamięci współdzielonej ...
               }

               a++; // do przerwania pętli cyklu 'życia' klienta
          }
     }

     return 0;
}