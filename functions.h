#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#define SHM_KEY 9876
#define SEM_KEY 5912
#define SEM_KEY_PASSENGERS 5678
#define SEM_KEY_BIKES 5679
#define MAX_PASSENGERS 20
#define MAX_BIKES 5
#define MAX_TRAINS 4
#define TRAIN_ARRIVAL_TIME 8

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery
    int passengers_with_bikes; // pasazerowie z rowerami
    int semaphores_valid; // Flaga czy semafory są dostępne
} Data;

extern int sem_passengers;

void semaphore_wait(int semid)
{
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        perror("Blad blokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int semid)
{
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        perror("Blad odblokowania semafora");
        exit(EXIT_FAILURE);
    }
}

int semaphore_create(key_t key)
{
    int semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (semid == -1)
    {
        if (errno == EEXIST)
        {
            semid = semget(key, 1, 0600);
        }
        else
        {
            perror("Blad utworzenia semafora");
            exit(EXIT_FAILURE);
        }
    }
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("Blad inicjalizacji semafora");
        exit(EXIT_FAILURE);
    }
    return semid;
}

void semaphore_remove(int semid)
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("Blad usuwania semafora");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_create(int *memory)
{
    *memory = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (*memory < 0)
    {
        perror("Blad utworzenia segmentu pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_address(int memory, Data **data)
{
    *data = (Data *)shmat(memory, NULL, 0);
    if (*data == (void *)-1)
    {
        perror("Blad dostepu do segmentu pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_detach(Data *data)
{
    if (shmdt(data) == -1)
    {
        perror("Blad odlaczenia pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_remove(int memory) 
{
    if (shmctl(memory, IPC_RMID, NULL) == -1) 
    {
        perror("Blad usuwania segmentu pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

int run_for_Ttime()
{
    time_t start_time = time(NULL);
    time_t current_time;
    while (1)
    {
        current_time = time(NULL);
        if (current_time - start_time >= 10)
        {
            return 1;
        }
    }
}

#endif // FUNCTIONS_H