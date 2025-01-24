#include "header_file.h"

// W PROGRAMIE JEST ZAŁOŻENIE, ŻE KLIENCI PRZYNOSZĄ PIENIĄDZE NA WIZYTĘ

// PONADTO ZAKŁADAM, ŻE PŁACĄ WIĘCEJ NIŻ TRZEBA

// ZAKŁADAM, ŻE PŁACĄ W KAŻDYM NOMINALE, BY UMOŻLIWIĆ PÓŹNIEJ WYDAWANIE,
// NAWET JEŚLI KASA BYŁA NA POCZĄTKU PUSTA

int sprawdz_czy_jest_reszta(int reszta_dla_klienta, Banknoty *Kasa);
void *watek_wydaje_reszte(void *arg);
int *wydaj_reszte(int kwota_reszty, Banknoty *Kasa);

Banknoty KASA = {0, 0, 0}; // inicjalizacja kasy

// const int LICZBA_WATKOW = 100;    // zakładam, że wystarczy 100 watków
int FAKTYCZNA_LICZBA_WATKOW = 0; // do thread_join()

pthread_t watki[100]; // zakładam, że wystarczy 100 watków

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int msqid_change;

int main(void)
{
  int LICZBA_TRANSAKCJI = 300;

  // SEMAFOR GLOBALNY do chronologii wstępnej

  int semID;
  int N = 5;

  semID = alokujSemafor(KEY_GLOB_SEM, N, IPC_CREAT | 0666);

  // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW fryjer -> kasa
  // zapłata za usługę od klienta i reszta do wydania dla klienta przez kasę

  int msqid_cash;

  if ((msqid_cash = msgget(MSG_KEY_CASH, 0666 | IPC_CREAT)) == -1)
  {
    perror("msgget - cash");
    exit(1);
  }

  // UZYSKIWANIE DOSTĘPU DO KOLEJKI KOMUNIKATÓW kasa -> klient
  // reszta do wydania dla klienta przez kasę
  // (lub ewentualnie informacja, że jej nie ma - bo wtedy
  // proces klient utkwi na msgrcv)

  if ((msqid_change = msgget(MSG_KEY_CHANGE, 0666 | IPC_CREAT)) == -1)
  {
    perror("msgget - change");
    exit(1);
  }

  int transakcje = 0;

  waitSemafor(semID, 2, SEM_UNDO); // CZEKAJ NA SEMAFORZE 2

  printf("[PROCEDURA serwer_kasa]\n");

  printf("W Kasie jest:\n");
  printf("banknoty 50 zl: %d\n", KASA.banknot50);
  printf("banknoty 20 zl: %d\n", KASA.banknot20);
  printf("banknoty 10 zl: %d\n", KASA.banknot10);

  struct cash klient_wplacil; // do przyjmowania wpłat od klienta (za pośrednictwem fryzjera)

  int wplaty_od_klientow = 0;

  int dotarla_gotowka = 0;

  int reszta_0_zl = 0;

  int reszta_faktyczna = 0;

  while (transakcje < LICZBA_TRANSAKCJI)
  {

    int sprawdzam_kolejke = 0;

    while (sprawdzam_kolejke < 5) // by nie czekać w nieskończoność na płatność
    {                             // czyli nie utkwić na msgrcv

      // KASA kilkukrotnie sprawdza kolejkę komunikatów
      if (msgrcv(msqid_cash, &klient_wplacil, 6 * sizeof(int), CASH_REGISTER, IPC_NOWAIT) == -1)
      {
        // perror("msgrcv - kasa");
        sprawdzam_kolejke++;
        usleep(10);
      }
      else
      {
        // printf("[ kasa ] otrzymała płatność od: %d\n", klient_wplacil.klient_PID);
        sprawdzam_kolejke = 6; // ZAKOŃCZ PĘTLĘ czekania na płatność
        wplaty_od_klientow++;
        dotarla_gotowka = 1;
      }
      usleep(50);
    }

    sprawdzam_kolejke = 0; // by w kolejnych obiegach pętli czekać na płatność

    if (dotarla_gotowka == 1)
    {

      // SYGNAŁ (ROZGŁOSZENIE - BROADCAST) DLA WĄTKÓW

      // printf("Wpłynęła kolejna wpłata do kasy \n");

      pthread_cond_broadcast(&cond);

      // printf("[ kasa ] Do zwrotu klientowi: %d\n", klient_wplacil.reszta_dla_klienta);

      // SEMAFOR DOSTĘPU DO KASY

      KASA.banknot50 += klient_wplacil.banknoty[0];
      KASA.banknot20 += klient_wplacil.banknoty[1];
      KASA.banknot10 += klient_wplacil.banknoty[2];

      struct change wydanie_reszty;

      // TRZEBA NAJPIERW POPRAWNIE PRZYPISAĆ WARTOŚCI DO wydanie_reszty

      wydanie_reszty.mtype = klient_wplacil.klient_PID;

      if (klient_wplacil.reszta_dla_klienta == 0) // JEŚLI RESZTY NIE MA ( wynosi 0 ZŁ )
      {
        if (msgsnd(msqid_change, &wydanie_reszty, 6 * sizeof(int), 0) == -1)
        {
          perror("msgsnd - kasa - reszta 0 zl\n");
          exit(1);
        }
        else
        {
          // printf("[ kasa ] wysłała resztę ' 0 ZŁ ' dla klienta.\n");
          reszta_0_zl++;
        }
      }
      else // JEŚLI RESZTA JEST ( reszta > 0 ZŁ)
      {
        // SIĘGANIE DO KASY POWINNO BYĆ ZABEZPIECZONE SEMAFOREM
        int wynik_sprawdzenia = sprawdz_czy_jest_reszta(klient_wplacil.reszta_dla_klienta, &KASA);
        // PONIEWAŻ WĄTKI TEŻ MOGĄ SIĘGAĆ DO KASY

        if (wynik_sprawdzenia == 0) // JEŚLI RESZTĘ MOŻNA OD RAZU WYDAĆ
        {
          wydanie_reszty.reszta = klient_wplacil.reszta_dla_klienta;
          // printf("[ kasa ]  wydać resztę, wydaję kwotę: %d \n", wydanie_reszty.reszta);
          int *do_wydania = wydaj_reszte(wydanie_reszty.reszta, &KASA);
          wydanie_reszty.banknoty[0] = do_wydania[0];
          wydanie_reszty.banknoty[1] = do_wydania[1];
          wydanie_reszty.banknoty[2] = do_wydania[2];
          free(do_wydania); // PAMIEĆ ZAALOKOWANIA W FUNKCJI wydaj_reszte()

          if (msgsnd(msqid_change, &wydanie_reszty, 6 * sizeof(int), 0) == -1)
          {
            perror("msgsnd - kasa - reszta\n");
            exit(1);
          }
          else
          {
            // printf("[ kasa ] wysłała resztę dla klienta.\n");
            reszta_faktyczna++;
          }

        }
        else // JEŚLI RESZTY NIE MOŻNA WYDAĆ, UTWÓRZ WĄTEK WYDAWANIA
        {
          // printf("[ kasa ] - brak reszty w kasie, uruchamiam wątek\n");
          struct change wydaje_watek;
          wydaje_watek.mtype = klient_wplacil.klient_PID;
          printf("Wysyłam do wątku PID klienta: %lu\n", wydaje_watek.mtype);
          wydaje_watek.reszta = klient_wplacil.reszta_dla_klienta;
          // printf("Do watku wysyłam dane:\n");
          // printf("Reszta do wypłacenia %d:\n", wydaje_watek.reszta);
          // printf("Adresat wypłaty %lu:\n",wydaje_watek.mtype );
          // zakładam, że wystarczy 10 watków
          int tworzenie_watku = pthread_create(&watki[FAKTYCZNA_LICZBA_WATKOW], NULL, watek_wydaje_reszte, &wydaje_watek);
          FAKTYCZNA_LICZBA_WATKOW++;
          if (tworzenie_watku == -1)
          {
            perror("kasa - pthread_create()\n");
            exit(1);
          }
        }
      }
    }

    dotarla_gotowka = 0; // zerowanie przed odbiorem kolejnej wiadomości od fryzjera

    // CZYSTO TESTOWE 'WZBOGACENIE KASY' BY SPRÓBOWAĆ ZAMKNĄĆ WĄTKI
    // /*
    KASA.banknot50 += 0;
    KASA.banknot20 += 2;
    KASA.banknot10 += 2;
    // */

    transakcje++;
  }

  for (int i = 0; i < FAKTYCZNA_LICZBA_WATKOW; i++)
  {
    if (pthread_join(watki[i], NULL) != 0)
    {
      perror("Błąd przyłączenia wątku");
    }
  }

  printf("[ kasa ] wpłaty od klientów - łącznie wpłat: %d\n", wplaty_od_klientow);
  printf("[ kasa ] RESZTY:\n");
  printf("[ kasa ] reszta 0 zl: %d\n", reszta_0_zl);
  printf("[ kasa ] reszta od razu: %d\n", reszta_faktyczna);
  printf("Reszty inne niż 0 zł wypłacił wątek\n");

  return 0;
}

void *watek_wydaje_reszte(void *arg) // FUNKCJA DLA WĄTKÓW
{
  struct change *reszta_watek = (struct change *)arg;
  // printf("Watek otrzymał: PID %ld, reszta %d\n", reszta_watek ->mtype, reszta_watek ->reszta);
  // semafor dostępu do KASY

  printf("Do watku dotarły dane:\n");
  printf("Reszta do wypłacenia: %d\n", reszta_watek->reszta);
  printf("Adresat wypłaty: %lu\n", reszta_watek->mtype);

  pthread_mutex_lock(&mutex);
  int reszta_w_watku = reszta_watek->reszta;
  int czy_mozna_wydac = sprawdz_czy_jest_reszta(reszta_w_watku, &KASA);
  while (czy_mozna_wydac == 1)
  {
    czy_mozna_wydac = sprawdz_czy_jest_reszta(reszta_w_watku, &KASA);
     printf("\t[watek ] czekam na odpowiednie nominały\n");
    pthread_cond_wait(&cond, &mutex);
  }
  // MOŻNA WYDAĆ RESZTĘ PRZEZ WĄTEK:

  int *do_wydania = wydaj_reszte(reszta_w_watku, &KASA);

  struct change wydanie_reszty_watkiem;

  wydanie_reszty_watkiem.mtype = reszta_watek->mtype;

  printf("[watek] będę wysyłać resztę do: %lu\n", wydanie_reszty_watkiem.mtype);

  wydanie_reszty_watkiem.banknoty[0] = do_wydania[0];
  // printf("[watek] reszta, 50 zl: %d\n", wydanie_reszty_watkiem.banknoty[0]);
  wydanie_reszty_watkiem.banknoty[1] = do_wydania[1];
  // printf("[watek] reszta, 20 zl: %d\n", wydanie_reszty_watkiem.banknoty[1]);
  wydanie_reszty_watkiem.banknoty[2] = do_wydania[2];
  // printf("[watek] reszta, 10 zl: %d\n", wydanie_reszty_watkiem.banknoty[2]);
  free(do_wydania); // PAMIEĆ ZAALOKOWANIA W FUNKCJI wydaj_reszte()

  pthread_mutex_unlock(&mutex);
  // semafor dostępu do KASY

  if (msgsnd(msqid_change, &wydanie_reszty_watkiem, 6 * sizeof(int), 0) == -1)
  {
    perror("msgsnd - watek - reszta\n");
    exit(1);
  }
  else
  {
    printf("[watek] wydał resztę \n");
  }
}

int sprawdz_czy_jest_reszta(int reszta_dla_klienta, Banknoty *Kasa)
{
  int reszta[3] = {0}; // Liczba banknotów każdego nominału wydanych jako reszta

  int reszta_w_kwocie = reszta_dla_klienta;

  // Wydawanie banknotów 50 zł
  while (reszta_dla_klienta >= 50 && Kasa->banknot50 > 0)
  {
    reszta_dla_klienta -= 50;
    Kasa->banknot50--;
    reszta[0]++;
  }

  // Wydawanie banknotów 20 zł
  while (reszta_dla_klienta >= 20 && Kasa->banknot20 > 0)
  {
    reszta_dla_klienta -= 20;
    Kasa->banknot20--;
    reszta[1]++;
  }

  // Wydawanie banknotów 10 zł
  while (reszta_dla_klienta >= 10 && Kasa->banknot10 > 0)
  {
    reszta_dla_klienta -= 10;
    Kasa->banknot10--;
    reszta[2]++;
  }

  if (reszta_dla_klienta > 0)
  {
    // printf("[ kasa ] zabrakło banknotów w kasie.");
    return 1; // Zwraca 1 gdy nie można wydać reszty
  }
  else
  {
    // printf("[ kasa ] Można wydać resztę\n");
    return 0; // Zwraca 0, jeśli resztę można wydać poprawnie
  }
}

int *wydaj_reszte(int kwota_reszty, Banknoty *Kasa) // WYDANIE RESZTY (USTALENIE NOMINAŁÓW)
{

  printf("Funkcja wydawania reszty, będę wydawać kwotę: %d\n", kwota_reszty);

  // Alokacja pamięci na tablicę
  int *do_wydania = (int *)malloc(3 * sizeof(int));
  if (do_wydania == NULL)
  {
    fprintf(stderr, "Błąd alokacji pamięci.\n");
    exit(1);
  }

  do_wydania[0] = 0; // Liczba banknotów 50 zł
  do_wydania[1] = 0; // Liczba banknotów 20 zł
  do_wydania[2] = 0; // Liczba banknotów 10 zł

  // Wydawanie banknotów 50 zł
  while (kwota_reszty > 49)
  {
    kwota_reszty -= 50;
    Kasa->banknot50--;
    do_wydania[0] = do_wydania[0] + 1;
  }

  // printf("Kwota reszty po pętli 50 zl: %d\n",kwota_reszty);

  // Wydawanie banknotów 20 zł
  while (kwota_reszty > 19)
  {
    kwota_reszty -= 20;
    Kasa->banknot20--;
    do_wydania[1] = do_wydania[1] + 1;
  }

  // printf("Kwota reszty po pętli 20 zl: %d\n",kwota_reszty);

  // Wydawanie banknotów 10 zł
  while (kwota_reszty > 9)
  {
    kwota_reszty -= 10;
    Kasa->banknot10--;
    do_wydania[2] = do_wydania[2] + 1;
  }

  // printf("Kwota reszty po pętli 10 zl: %d\n",kwota_reszty);

  // printf("do wydania z funkcji: 50 zl: %d\n", do_wydania[0]);
  // printf("do wydania z funkcji: 20 zl: %d\n", do_wydania[1]);
  // printf("do wydania z funkcji: 10 zl: %d\n", do_wydania[2]);

  return do_wydania;
}