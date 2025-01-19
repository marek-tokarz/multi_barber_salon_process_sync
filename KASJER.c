#include "header_file.h"

int main(void)
{

    // SEMAFOR GLOBALNY DO CHRONOLOGII URUCHOMIENIA

    // KLIENT -> klient -> poczekalnia -> FRYZJER -> fryzjer

    int semID;
    int N = 5; // liczba semaforow

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | IPC_EXCL | 0666);

    int i;

    for (i = 0; i < N; i++)
    {
        inicjalizujSemafor(semID, i, 0); // inicjalizujemy zerami
        // int wartoscSemafora = semctl(semID, 0, GETVAL);
        // printf("Wartość semafora: %d\n", wartoscSemafora);
    }

    // TWORZENIE PROCEDURY: PROCEDURA_KLIENT

    int KLIENT_pid;

    KLIENT_pid = fork();
    switch (KLIENT_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);
    case 0: // Proces potomny - uruchomienie nowego programu
        execl("./PROCEDURA_KLIENT", "PROCEDURA_KLIENT", (char *)NULL);
        perror("exec: PROCEDURA_KLIENT failed"); // Jeśli execlp nie zadziałało
        exit(1);
    default: // Proces macierzysty
        break;
    }

    // TWORZENIE PROCEDURY: serwer_poczekalnia

    int  serwer_poczekalnia_pid;

    serwer_poczekalnia_pid = fork();
    switch (serwer_poczekalnia_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);
    case 0: // Proces potomny - Uruchomienie nowego programu
        execl("./serwer_poczekalnia", "serwer_poczekalnia", (char *)NULL);
        perror("exec: serwer_poczekalnia failed");  
        exit(1); // Jeśli execlp nie zadziałało
    default: // Proces macierzysty
        break;
    }

    // TWORZENIE PROCEDURY: PROCEDURA_FRYZJER

    pid_t FRYZJER_pid;

    FRYZJER_pid = fork();
    switch (FRYZJER_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);
    case 0: // Proces potomny - uruchomienie nowego programu
        execl("./PROCEDURA_FRYZJER", "PROCEDURA_FRYZJER", (char *)NULL);
        perror("exec: PROCEDURA_FRYZJER failed"); // Jeśli execlp nie zadziałało
        exit(1);
    default: // Proces macierzysty
        break;
    }

    int status;
    pid_t zakonczony_pid;

    zakonczony_pid = waitpid(KLIENT_pid, &status, 0);
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for KLIENT");
    }
    else
    {
        printf("PROCEDURA_KLIENT zakończona\n");
    }

    zakonczony_pid = waitpid(FRYZJER_pid, &status, 0);
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for FRYZJER");
    }
    else
    {
        printf("PROCEDURA_FRYZJER zakończona\n");
    }

    zakonczony_pid = waitpid(serwer_poczekalnia_pid, &status, 0);
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for serwer_poczekalnia_pid");
    }
    else
    {
        printf("serwer_poczekalnia_pid zakończona\n");
    }

    zwolnijSemafor(semID, N);

    return 0;
}