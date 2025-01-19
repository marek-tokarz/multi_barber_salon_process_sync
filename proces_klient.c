#include "header_file.h"

int main(void)
{
     printf("proces klient: %d\n", getpid());

     int msqid;
     struct msgbuf buf;
     pid_t my_pid = getpid();

     msqid = msgget(MSG_KEY, 0666);
     if (msqid == -1)
     {
          perror("msgget");
          exit(1);
     }

     int a = 0;

     while (a < 3)
     {
          // ... Twój kod, który wykonuje się w pętli ...
          // printf("Proces %d wykonuje swoje zadanie...\n", my_pid);
          usleep(1000); // Przykładowe działanie

          // Wysyłanie PID do kolejki komunikatów
          buf.mtype = my_pid; // Ustawiamy PID jako typ wiadomości
          buf.pid = my_pid;
          if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1)
          {
               perror("msgsnd - klient_proces");
               exit(1);
          }
          else
          {
               printf("Proces %d wysłał swój PID do kolejki.\n", my_pid);
          }

          usleep(100);

          a++;
     }

     return 0;
}