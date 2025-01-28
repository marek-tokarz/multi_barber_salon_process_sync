#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <errno.h>

// TWORZENIE SEMAFORA
int alokujSemafor(key_t klucz, int number, int flagi)
{
   int semID;
   if ( (semID = semget(klucz, number, flagi)) == -1)
   {
      perror("Blad semget (alokujSemafor): ");
      exit(1);
   }
   return semID;
}

// WSTAWIANIE ZER - WSTĘPNE OPUSZCZENIE SEMAFORA
void inicjalizujSemafor(int semID, int number, int val)
{
   if ( semctl(semID, number, SETVAL, val) == -1 )
   {
      perror("Blad semctl (inicjalizujSemafor): ");
      exit(1);
   }
}

// CZEKAJ NA SEMAFORZE
int waitSemafor(int semID, int number, int flags)
{
   struct sembuf operacje[1];
   operacje[0].sem_num = number;
   operacje[0].sem_op = -1;
   operacje[0].sem_flg = SEM_UNDO;
   
   if ( semop(semID, operacje, 1) == -1 )
   {
      perror("Blad semop (waitSemafor)");
      fprintf(stderr, "errno: %d\n", errno);
      // return -1;
   }
   
   return 1;
}

// PODNIEŚ SEMAFOR
void signalSemafor(int semID, int number)
{
   struct sembuf operacje[1];
   operacje[0].sem_num = number;
   operacje[0].sem_op = 1;
   //operacje[0].sem_flg = SEM_UNDO;

   if (semop(semID, operacje, 1) == -1 )
      perror("Blad semop (postSemafor): ");

   return;
}

// USUNIĘCIE SEMAFORA
int zwolnijSemafor(int semID, int number)
{
   return semctl(semID, number, IPC_RMID, NULL);
}