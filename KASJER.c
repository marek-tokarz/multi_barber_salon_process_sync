#include "header_file.h"

int main(void)
{

    // TWORZENIE PROCEDURY: PROCEDURA_FRYZJER

    pid_t FRYZJER_pid;

    FRYZJER_pid = fork();
    switch (FRYZJER_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);

    case 0: // Proces potomny
        // Uruchomienie nowego programu
        execl("PROCEDURA_FRYZJER", "PROCEDURA_FRYZJER", (char *)NULL);
        perror("exec: PROCEDURA_FRYZJER failed");  // Jeśli execlp nie zadziałało
        exit(1);

    default: // Proces macierzysty
        break;
    }

    // TWORZENIE PROCEDURY: PROCEDURA_FRYZJER

    int  KLIENT_pid;

    KLIENT_pid = fork();
    switch (KLIENT_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);

    case 0: // Proces potomny
        // Uruchomienie nowego programu
        execl("PROCEDURA_KLIENT", "PROCEDURA_KLIENT", (char *)NULL);
        perror("exec: PROCEDURA_KLIENT failed");  // Jeśli execlp nie zadziałało
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

}