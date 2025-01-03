#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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

int main()
{
    signal(SIGINT, handle_sigint);

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

    while (running)
    {
        data->passengers_waiting++; // zwiekszenie liczby oczekujacych pasazerow
        int has_bike = rand() % 2; // czy pasazer posiada rower
        printf("PASAZER: Nowy pasazer na peronie.\nLiczba wszystkich pasazerow: %d, pasazerow z rowerami: %d\n", data->passengers_waiting, has_bike);
        sleep(1);
    }

    data->generating = 0; // flaga generowanie zakonczone
    shmdt(data); // odlaczenie pamieci dzielonej
    return 0;
}