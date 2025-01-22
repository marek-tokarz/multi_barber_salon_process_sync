#include "header_file.h"

int sprawdz_czy_jest_reszta(int reszta_dla_klienta, Banknoty *Kasa);

/*
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int sprawdz_reszte(int reszta)
{}

void *funkcja_watku(void *arg)
{
    int *reszta_watek = (int *)arg;
    while ((sprawdz_reszte(reszta_watek)) == 0)
    {
      printf("Brak reszty\n");
      pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_lock(&mutex);
    // operacja wydania reszty i wyslania jej do klienta
    pthread_mutex_unlock(&mutex);

}

void *rob(void *arg)
{
  Watek *wat = (Watek *)arg;
  printf("Watek otrzymał %d, %d\n", wat->nr, wat->suma);
  pthread_exit(0);
}
*/

int main(void)
{
  int LICZBA_TRANSAKCJI = 200;

  // SEMAFOR GLOBALNY do chronologii wstępnej

  printf("[PROCEDURA serwer_poczekalnia]\n");

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

  Banknoty KASA = {10, 10, 10};

  int transakcje = 0;

  // pthread_t watek1;

  // kolejka komunikatów od fryzjera

  // kolejka komunikatów do klienta

  // TUTAJ PRZYDAŁBY SIĘ SEMAFOR GLOBALNY, BY KASA ZACZĘŁA DZIAŁAĆ
  // GDY FRYZJERZY SĄ JUŻ UTWORZENI

  // printf("Kasa czeka na semaforze nr 2\n");

  waitSemafor(semID, 2, SEM_UNDO); // CZEKAJ NA SEMAFORZE 2

  struct cash klient_wplacil;

  int wplaty_od_klientow = 0;

  int dotarla_gotowka = 0;

  while (transakcje < LICZBA_TRANSAKCJI)
  {

    // msgrcv (OD FRYZJERA)

    int sprawdzam_kolejke = 0;

    while (sprawdzam_kolejke < 5) // by nie czekać w nieskończoność na płatność
    {                             // czyli nie utkwić na msgrcv

      // RACZEJ BEZ IPC_NOWAIT - fryzjer ma czekać na kasę od klienta
      if (msgrcv(msqid_cash, &klient_wplacil, 5 * sizeof(int), CASH_REGISTER, IPC_NOWAIT) == -1)
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
      // printf("[ kasa ] Do zwrotu klientowi: %d\n", klient_wplacil.reszta_dla_klienta);

      // SEMAFOR DOSTĘPU DO KASY

      KASA.banknot50 += klient_wplacil.banknoty[0];
      KASA.banknot20 += klient_wplacil.banknoty[1];
      KASA.banknot10 += klient_wplacil.banknoty[2];

      // sprawdź czy dla danej wpłaty od klienta można zwrócić resztę

      // sprawdz_reszte();

      if (klient_wplacil.reszta_dla_klienta == 0)
      {
        // WYŚLIJ 0 ZŁ DO KLIENTA, PONIEWAŻ CZEKA NA msgrcv
      }
      else
      {
        sprawdz_czy_jest_reszta(klient_wplacil.reszta_dla_klienta, &KASA);
      }
    }

    dotarla_gotowka = 0; // zerowanie przed odbiorem kolejnej wiadomości

    // if (sprawdz_reszte() == 1)
    // jeśli tak, to wyślij do klienta resztę w postaci banknotów o określonych nominałach
    // pthread_cond_signal(&cond);

    // else
    // uruchom wątek do zwrócenia reszty

    transakcje++;
  }

  printf("[ kasa ] wpłaty od klientów - łącznie wpłat: %d\n", wplaty_od_klientow);

  return 0;
}

int sprawdz_czy_jest_reszta(int reszta_dla_klienta, Banknoty *Kasa)
{
  int reszta[3] = {0}; // Liczba banknotów każdego nominału wydanych jako reszta

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
    printf("[ kasa ] zabrakło banknotów w kasie.");
    return reszta_dla_klienta; // Zwraca niewydaną resztę
  }
  else
  {
    printf("[ kasa ] Można wydać resztę\n");
    return 0; // Zwraca 0, jeśli resztę można wydać poprawnie
  }
}