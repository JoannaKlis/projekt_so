#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SHM_KEY 1234
#define MAX_TRAINS 4
#define MAX_PASSENGERS 10

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
} Data;

void init_shared_memory(int *shmid, Data **data) 
{
    *shmid = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (*shmid < 0) 
    {
        perror("ZARZADCA: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
    *data = (Data *)shmat(*shmid, NULL, 0);
    if (*data == (void *)-1)
    {
        perror("ZARZADCA: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void cleanup_shared_memory(int shmid, Data *data) // czyszczenie pamieci
{
    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
}

void station_master(Data *data, int N) 
{
    while (1)  // dzialanie zarzadcy
    {
        if (data->passengers_waiting == 0 && data->generating == 0) 
        {
            printf("ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n");
            break;
        }

        printf("ZARZADCA: Pociag %d przyjechal.\nLiczba wolnych miejsc: %d\n", data->current_train, data->free_seat);
        sleep(5); // czas na wsiadanie pasazerow

        printf("ZARZADCA: Pociag %d odjezdza.\n", data->current_train);
        data->current_train = (data->current_train + 1) % N;
        data->free_seat = 0; // reset wolnych miejsc

        sleep(2); // czas na przyjazd nowego pociągu
    }
}

int main() 
{
    int shmid;
    Data *data;

    init_shared_memory(&shmid, &data);

    data->current_train = 0; // inicjalizacja pamieci dzielonej
    data->free_seat = 0;
    data->passengers_waiting = 0; //ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani
    for (int i = 0; i < MAX_TRAINS; i++) 
    {
        for (int j = 0; j < MAX_PASSENGERS; j++) 
        {
            data->train_data[i][j] = 0; // 0 to wolne miejsce
        }
    }

    pid_t train_manager_pid = fork();
    if (train_manager_pid == 0)
    {
        execl("./train_manager", "./train_manager", NULL);
        perror("ZARZADCA: Blad execl pliku train_manager");
        exit(EXIT_FAILURE);
    }

    pid_t passenger_pid = fork();
    if (passenger_pid == 0) 
    {
        execl("./passenger", "./passenger", NULL);
        perror("ZARZADCA: Blad execl pliku passenger");
        exit(EXIT_FAILURE);
    }

    station_master(data, MAX_TRAINS);

    cleanup_shared_memory(shmid, data);
    return 0;
}