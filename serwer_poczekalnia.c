#include "header_file.h"

int main()
{
    int semID;
    int N = 5;

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

    waitSemafor(semID, 0, SEM_UNDO); // CZEKAJ NA SEMAFORZE 0

    printf("PROCEDURA serwer_poczekalnia\n");

    signalSemafor(semID, 1); // PODNIEŚ SEMAFOR 1
}

