#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

#define SHM_KEY 1234
#define MAX_PASSENGERS 10
#define SEM_KEY_PASSENGERS 5678
#define SEM_KEY_BIKES 5679
#define MAX_BIKES 5

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[4][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery
} Data;

void semaphore_wait(int semid) //blokowanie dostepu do semafora
{
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad blokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int semid) //odblokowanie semafora
{
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad odblokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void handle_passenger(int passenger_pid, int has_bike, Data *data, int sem_passengers, int sem_bikes) // zarzadzanie pasazerami i rowerami
{
    semaphore_wait(sem_passengers); // zablokowanie dostepu do danych pasazerow
    int train = data->current_train; //aktualny pociag

    if (has_bike) 
    {
        semaphore_wait(sem_bikes); // zablokowanie dostepu do danych rowerow
        if (data->free_seat < MAX_PASSENGERS && data->free_bike_spots > 0) 
        {
            data->train_data[train][data->free_seat] = passenger_pid; // dodanie pasazera do pociagu
            printf("KIEROWNIK POCIAGU: Pasazer %d z rowerem wszedl do pociagu %d i zajal miejsce %d.\n", passenger_pid, train, data->free_seat);
            data->free_seat++;
            data->free_bike_spots--;
        } else 
        {
            printf("KIEROWNIK POCIAGU: Pociag %d jest pelny.\nPasazer %d musi czekac.\n", train, passenger_pid);
        }
        semaphore_signal(sem_bikes); // odblokowanie semafora rowerow
    } else
    {
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
    semaphore_signal(sem_passengers); // Odblokowanie semafora pasazerow
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

    int sem_passengers = semget(SEM_KEY_PASSENGERS, 1, 0600); //semafor dla pasazerow
    if (sem_passengers == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad semafora dla pasazerow");
        exit(EXIT_FAILURE);
    }

    int sem_bikes = semget(SEM_KEY_BIKES, 1, 0600); // semafor dla rowerow
    if (sem_bikes == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad semafora dla rowerow");
        exit(EXIT_FAILURE);
    }

    while (data->passengers_waiting > 0 || data->generating) // zarzadzanie pasazerow
    {
        printf("KIEROWNIK POCIAGU: Rozpoczynam zarzadzanie.\n", data->passengers_waiting);

        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            int passenger_pid = rand() % 10000; // generowanie PIDu pasazera
            int has_bike = rand() % 2; // generowanie rowerow dla pasazerow
            handle_passenger(passenger_pid, has_bike, data, sem_passengers, sem_bikes); //obsluga pasazera w pociagu
            data->passengers_waiting--;

            if (data->passengers_waiting == 0 && !data->generating) 
            {
                printf("KIEROWNIK POCIAGU: Brak oczekujacych pasazerow. KONIEC DZIALANIA.\n");
                break;
            }
        }
        sleep(2);
    }
    shmdt(data); // odlaczenie pamieci dzielonej
    return 0;
}