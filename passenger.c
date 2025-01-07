#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define SHM_KEY 1234
#define MAX_PASSENGERS 10

typedef struct
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[4][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery
} Data;

int running = 1; // flaga na petle generujaca pasazerow

void handle_sigint(int sig) //ctrl+c przerywa generowanie
{
    printf("PASAZER: Otrzymalem sygnal.\nKoniec generowania pasazerow.\n");
    running = 0; // flaga na 0 przerywa petle
}

void notify_disembarkation(int train, int station) 
{
    printf("PASAZER: Pociag %d wysadzil pasazerow na stacji %d.\n", train, station);
}

int main()
{
    signal(SIGINT, handle_sigint);
    srand(time(NULL));
    int max_waiting_passengers = 100; //tymczasowy stoper pasazerow

    int shmid = shmget(SHM_KEY, sizeof(Data), 0600); // identyfikator pamieci dzielonej
    if (shmid < 0) 
    {
        perror("PASAZER: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    Data *data = (Data *)shmat(shmid, NULL, 0); // dolaczenie segmentu pamieci dzielonej
    if (data == (void *)-1) 
    {
        perror("PASAZER: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    while (running && data->passengers_waiting <= max_waiting_passengers)
    {
        int passengers_to_generate = 10 + rand() % 11; // losowa liczba pasazerow od 10 do 20
        data->passengers_waiting += passengers_to_generate; // zwiekszenie liczby oczekujacych pasazerow
        int passengers_with_bikes = rand() % (passengers_to_generate + 1); // pasazerowie z rowerami
        data->free_bike_spots = passengers_with_bikes; // zwiekszenie liczby pasazerow z rowerami
        printf("PASAZER: Wygenerowano %d nowych pasazerow.\n", passengers_to_generate);

        if (data->current_train == 2) //stacja 2
        {
            notify_disembarkation(data->current_train, 2);
        }

        sleep(2);
    }

    if (data->passengers_waiting > max_waiting_passengers) 
    {
        printf("PASAZER: Liczba oczekujących pasazerow przekroczyla 100. KONIEC DZIALANIA.\n");
    }

    data->generating = 0; // flaga generowanie zakonczone
    shmdt(data); // odlaczenie pamieci dzielonej
    return 0;
}