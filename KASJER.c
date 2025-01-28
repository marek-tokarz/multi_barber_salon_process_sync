#include "header_file.h"

int main(int argc, char *argv[])
{

    // CZAS OTWARCIA JAKO ARGUMENT

    int seconds = atoi(argv[1]);

    if (argc != 2)
    {
        fprintf(stderr, "Użycie: %s <ilość_sekund>\n", argv[0]);
        
        exit(1);
    }

    // KOLEJKA KOMUNIKATÓW DO OBSŁUGI SYGNAŁÓW

    int msgqid_barber;

    msgqid_barber = msgget(KEY_SIG1_BARBER, IPC_CREAT | 0666);
    if (msgqid_barber == -1)
    {
        perror("msgget msgqid_barber failed");
        exit(1);
    }

    // SEMAFOR kasy

    int semID_kasa;

    semID_kasa = alokujSemafor(KEY_SEM_KASA, 1, IPC_CREAT | 0666);

    // SEMAFOR GLOBALNY DO CHRONOLOGII URUCHOMIENIA

    // KLIENT -> klient -> poczekalnia -> FRYZJER -> fryzjer

    int semID; // numer semafora globalnego

    // 3 - OD PROCEDURA FRYZJER (fryzjerzy zniknęli)
    // 4 - OD PROCEDURA KLIENT (klienci zniknęli)

    semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | IPC_EXCL | 0666);

    int i;

    for (i = 0; i < N; i++)
    {
        inicjalizujSemafor(semID, i, 0); // inicjalizujemy zerami SEMAFOR GLOBALNY
        // int wartoscSemafora = semctl(semID, 0, GETVAL);
        // printf("Wartość semafora: %d\n", wartoscSemafora);
    }

    // PAMIĘĆ WSPÓŁDZIELONA

    // TWORZENIE PAMIĘCI WSPÓŁDZIELONEJ

    int shmid; // nr pamięci współdzielonej - poczekalnia

    // Tworzenie segmentu pamięci współdzielonej
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | IPC_EXCL | 0666);

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
        perror("exec: PROCEDURA_KLIENT failed");
        exit(1); // Jeśli execlp nie zadziałało
    default:     // Proces macierzysty
        break;
    }

    // TWORZENIE PROCEDURY: serwer_poczekalnia

    int serwer_poczekalnia_pid;

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
    default:     // Proces macierzysty
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
        perror("exec: PROCEDURA_FRYZJER failed");
        exit(1); // Jeśli execlp nie zadziałało
    default:     // Proces macierzysty
        break;
    }

    // TWORZENIE PROCEDURY: serwer_kasa

    int serwer_kasa_pid;

    serwer_kasa_pid = fork();
    switch (serwer_kasa_pid)
    {
    case -1: // Błąd przy fork
        perror("fork failed");
        exit(1);
    case 0: // Proces potomny - Uruchomienie nowego programu
        execl("./serwer_kasa", "serwer_kasa", (char *)NULL);
        perror("exec: serwer_kasa failed");
        exit(1); // Jeśli execlp nie zadziałało
    default:     // Proces macierzysty
        break;
    }

    // printf("[KASJER] czekam na semaforze 5\n");

    waitSemafor(semID, 5, SEM_UNDO); // CZEKAJ NA SEMAFORZE 5

    int fryzjerzy_pid[LICZBA_FRYZJEROW];

    // Odbieranie komunikatów z kolejki (PID fryzjerów)
    // printf("[KASJER] czekam na PIDy fryzjerów\n");
    for (int i = 0; i < LICZBA_FRYZJEROW; i++)
    {
        MessageBarber msg;
        if (msgrcv(msgqid_barber, &msg, sizeof(msg.pid), 1, 0) == -1)
        {
            perror("msgrcv KASJER failed");
            // exit(1);
        }
        fryzjerzy_pid[i] = msg.pid;
        // printf("[KASJER] odebrał PID fryzjera %d\n", msg.pid);
    }

    srand(getpid());

    printf("[KASJER] zwalniam do domu losowego fryzjera\n");
    int nr_fryzjera = rand() % LICZBA_FRYZJEROW;
    int zwalniany_fryzjer = fryzjerzy_pid[nr_fryzjera];
    // printf("Nr kolejny zwalnianego fryzjera: %d\n",nr_fryzjera);
    printf("[KASJER] PID zwalnianego fryzjera: %d\n", zwalniany_fryzjer);
    kill(zwalniany_fryzjer, SIGUSR1);

    // TESTOWO ZWOLNIJ WSZYSTKICH FRYZJERÓW:
    /*
    for (int i = 0; i < LICZBA_FRYZJEROW; i++)
    {
        kill(fryzjerzy_pid[i], SIGUSR1);
        printf("[KASJER] PID zwalnianego fryzjera: %d\n", fryzjerzy_pid[i]);
    }
    */

    

    printf("[KASJER] ewakuuję poczekalnię\n");
    kill(serwer_poczekalnia_pid, SIGUSR2);

    
        // PĘTLA WYZNACZAJĄCA CZAS DZIAŁANIA SALONU
    // czas w sekundach 'n' jako argument: ./KASJER n

    time_t start_time, current_time;
     
    time(&start_time);

    do
    {
        time(&current_time);
        usleep(1000000);
    } while (difftime(current_time, start_time) < seconds);

    printf("[KASJER] zamykam kasę\n");
    kill(serwer_kasa_pid, SIGUSR1);

    printf("[KASJER] zamykam poczekalnię\n");
    kill(serwer_poczekalnia_pid, SIGUSR1);

    // ODBIERANIE STATUSÓW PROCESÓW POTOMNYCH i USUWANIE ZASOBÓW IPC

    int status;
    pid_t zakonczony_pid;

    zakonczony_pid = waitpid(serwer_kasa_pid, &status, 0); // czekanie na serwer_kasa
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for serwer_kasa_pid");
    }
    else
    {
        printf("[serwer_kasa] zakończona\n");
    }

    // KOLEJKA KOM. od fryzjera do kasy

    int msqid_cash;

    if ((msqid_cash = msgget(MSG_KEY_CASH, 0666 | IPC_CREAT)) == -1)
    {
        perror("[KASJER] msgget - cash");
        // exit(1);
    }

    msgctl(msqid_cash, IPC_RMID, NULL);

    // KOLEJKA KOM. klient <-> fryzjer

    int msqid_pay;

    if ((msqid_pay = msgget(MSG_KEY_PAY, 0666 | IPC_CREAT)) == -1)
    {
        perror("[KASJER] msgget");
        // exit(1);
    }

    msgctl(msqid_pay, IPC_RMID, NULL);

    // KOLEJKA KOM. od kasy do klienta

    int msqid_change;

    if ((msqid_change = msgget(MSG_KEY_CHANGE, 0666 | IPC_CREAT)) == -1)
    {
        perror("[KASJER] msgget - change");
        // exit(1);
    }

    msgctl(msqid_change, IPC_RMID, NULL);

    // KOLEJKA KOM. od klienta do poczekalni i z powrotem

    int msqid_wait_room;

    msqid_wait_room = msgget(MSG_KEY_WAIT_ROOM, 0666 | IPC_CREAT); // dostęp do kolejki kom. poczekalnia
    if (msqid_wait_room == -1)
    {
        perror("[KASJER] msgget msqid_wait_room");
        // exit(1);
    }

    msgctl(msqid_wait_room, IPC_RMID, NULL);

    zakonczony_pid = waitpid(serwer_poczekalnia_pid, &status, 0); // czekanie na serwer_poczekalnia
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for serwer_poczekalnia_pid");
    }
    else
    {
        printf("[serwer_poczekalnia] zakończona\n");
    }

    zakonczony_pid = waitpid(KLIENT_pid, &status, 0); // czekanie na PROCEDURA_KLIENT
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for KLIENT");
    }
    else
    {
        printf("[PROCEDURA_KLIENT] zakończona\n");
    }

    zakonczony_pid = waitpid(FRYZJER_pid, &status, 0); // czekanie na PROCEDURA_FRYZJER
    if (zakonczony_pid == -1)
    {
        perror("waitpid failed for FRYZJER");
    }
    else
    {
        printf("[PROCEDURA_FRYZJER] zakończona\n");
    }

    waitSemafor(semID, 3, SEM_UNDO); // SEMAFOR OD FRYZJERÓW
    waitSemafor(semID, 4, SEM_UNDO); // SEMAFOR OD KLIENTÓW

    zwolnijSemafor(semID, N); // USUWANIE SEMAFOR GLOBALNEGO do chronologii

    // DO USUNIĘCIA ZASOBÓW JAKIE POZOSTAŁY - jeśli inne programy ich nie zamkneły poprawnie

    msgctl(msgqid_barber, IPC_RMID, NULL);

    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("KASJER: shmget");
        exit(1);
    }

    int semID_shm; // numer semafora pamięci współdzielonej
    int B = 1;
    semID_shm = alokujSemafor(KEY_SHM_SEM, B, IPC_CREAT | 0666); // dostęp do semafora pamięci

    int semID_fotel = alokujSemafor(KEY_FOTEL_SEM, 1, 0666 | IPC_CREAT);

    shmctl(shmid, IPC_RMID, NULL);  // usunięcie pamięci współdzielonej
    zwolnijSemafor(semID_shm, 1);   // USUWANIE SEMAFOR pamieci wspóldzielonej
    zwolnijSemafor(semID_fotel, 0); // USUWANIE SEMAFORA foteli
    zwolnijSemafor(semID_kasa, 0);  // USUWANIE SEMAFORA KASY

    printf("[PROCEDURA KASJER] zakończona\n");

    return 0;
}