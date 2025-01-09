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
#define MAX_TRAINS 4

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
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

void handle_passenger(Data *data, int sem_passengers, int sem_bikes) // zarzadzanie pasazerami i rowerami
{
    sleep(3);
    printf("KIEROWNIK POCIAGU: Pasazerowie wsiadaja do pociagu %d.\n", data->current_train);

    while (data->passengers_waiting > 0 || data->generating) 
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            semaphore_wait(sem_passengers); // zablokowanie dostepu do danych pasazerow
            data->free_seat++;
            data->passengers_waiting--;
            semaphore_signal(sem_passengers); // odblokowanie semafora pasazerow
        }

            //int has_bike = rand() % 2; // czy pasazer ma rower
            /*if (has_bike) 
            {
                printf("ZARZADCA: Pasazer %d z rowerem wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            } else 
            {
                printf("ZARZADCA: Pasazer %d wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            }*/
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
        shmdt(data);
        semctl(sem_passengers, 0, IPC_RMID); // usuwanie semafora pasazerow jak wystapi blad
        exit(EXIT_FAILURE);
    }

    handle_passenger(data, sem_passengers, sem_bikes);
    shmdt(data); // odlaczenie pamieci dzielonej
    shmctl(shmid, IPC_RMID, NULL); // usuniecie segmentu pamieci dzielonej
    semctl(sem_passengers, 0, IPC_RMID); // usuniecie semafora pasazerow
    semctl(sem_bikes, 0, IPC_RMID); // usuniecie semafora rowerow
    return 0;
}