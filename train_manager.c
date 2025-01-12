#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_KEY 9876
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

int semaphore_create(key_t key)
{
    int semid = semget(key, 1, IPC_CREAT | 0600);
    if (semid == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad utworzenia semafora");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad inicjalizacji semafora");
        exit(EXIT_FAILURE);
    }
    return semid;
}

void semaphore_remove(int semid)
{
    if (semctl(semid, 0, IPC_RMID) == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad usuwania semafora");
        exit(EXIT_FAILURE);
    }
}

void handle_passenger(Data *data, int sem_passengers) // zarzadzanie pasazerami
{
    printf("KIEROWNIK POCIAGU: Oczekiwanie na pociag: %d.\n", data->current_train);

    while (data->passengers_waiting == 0 && data->generating) // oczekiwanie na wygenerowanie pasazerow
    {
        sleep(1);
    }
    printf("KIEROWNIK POCIAGU: Rozpoczynam zarzadzanie pasazerami dla pociagu %d.\n", data->current_train);

    while (data->passengers_waiting > 0 || data->generating) 
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            semaphore_wait(sem_passengers); // zablokowanie dostepu do danych pasazerow
            int seat = data->free_seat++;
            data->train_data[data->current_train][seat] = getpid(); // PID pasazera
            data->passengers_waiting--;
            printf("KIEROWNIK POCIAGU: Pasazer wsiadl do pociagu %d i zajal miejsce %d.\n", data->current_train, seat);
            semaphore_signal(sem_passengers); // odblokowanie semafora pasazerow
        }
        sleep(1);

            //int has_bike = rand() % 2; // czy pasazer ma rower
            /*if (has_bike) 
            {
                printf("ZARZADCA: Pasazer %d z rowerem wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            } else 
            {
                printf("ZARZADCA: Pasazer %d wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            }*/
    }
    printf("KIEROWNIK POCIAGU: Brak oczekujacych pasazerow\nKONIEC DZIALANIA.\n");
}

int memory, detach;
Data *data = NULL; // globalny wskaxnik dla pamieci dzielonej

void shared_memory_create()
{
    memory = shmget(SHM_KEY, sizeof(Data), 0600);
    if (memory < 0)
    {
        perror("KIEROWNIK POCIAGU: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}


void shared_memory_address()
{
    data = (Data *)shmat(memory, NULL, 0);
    if (data == (void *)-1)
    {
        perror("KIEROWNIK POCIAGU: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_detach()
{
    detach = shmdt(data);
    if (detach == -1) 
    {
        perror("KIEROWNIK POCIAGU: Blad odlaczenia pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    shared_memory_create();
    shared_memory_address();

    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);
    handle_passenger(data, sem_passengers);

    shared_memory_detach();
    semaphore_remove(sem_passengers);
    return 0;
}