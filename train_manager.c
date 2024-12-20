#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define SHM_KEY 1234
#define MAX_PASSENGERS 10

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[4][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
} Data;

void handle_passenger(int passenger_pid, Data *data) // zarzadzanie pasazerami
{
    int train = data->current_train;
    if (data->free_seat < MAX_PASSENGERS)
    {
        data->train_data[train][data->free_seat] = passenger_pid;
        printf("KIEROWNIK POCIAGU: Pasazer %d wszedl do pociagu %d i zajal miejsce %d.\n", passenger_pid, train, data->free_seat);
        data->free_seat++;
    } else
    {
        printf("KIEROWNIK POCIAGU: Pociag %d jest pelny.\nPasazer %d musi czekac.\n", train, passenger_pid);
    }
}

int main()
{
    int shmid = shmget(SHM_KEY, sizeof(Data), 0600);
    if (shmid < 0)
    {
        perror("KIEROWNIK POCIAGU: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    Data *data = (Data *)shmat(shmid, NULL, 0);
    if (data == (void *)-1)
    {
        perror("KIEROWNIK POCIAGU: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    while (1) // tworzenie pasazerow
    {
        int passenger_pid = rand() % 10000; // generowanie PIDu pasazera
        handle_passenger(passenger_pid, data);
        sleep(1);
    }

    shmdt(data);
    return 0;
}