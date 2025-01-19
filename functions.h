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
#include <sys/stat.h>
#include <dirent.h>

#define SHM_KEY 9876
#define SEM_KEY 5912
#define SEM_KEY_PASSENGERS 5678
#define SEM_KEY_BIKES 5679
#define MAX_PASSENGERS 20
#define MAX_BIKES 5
#define MAX_TRAINS 4
#define TRAIN_DEPARTURE_TIME 12
#define TRAIN_ARRIVAL_TIME 4

typedef struct 
{
    short current_train; // aktualny pociag na stacji
    short free_seat;     // numer 1 wolnego miejsca w pociagu
    short train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    short passengers_waiting; // liczba oczekujacych pasazerow
    short generating; // flaga czy pasazerowie sa generowani
    short free_bike_spots; // liczba wolnych miejsc na rowery
    short passengers_with_bikes; // pasazerowie z rowerami
} Data;

extern int sem_passengers;

void handle_error(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

size_t calculate_directory_size(const char *path)
{
    size_t total_size = 0;
    struct stat st;
    struct dirent *entry;

    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("Blad otwierania katalogu");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &st) == 0)
        {
            if (S_ISREG(st.st_mode)) // tylko pliki regularne
                total_size += st.st_size;
        }
    }

    closedir(dir);
    return total_size;
}

void semaphore_wait(int semid)
{
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        handle_error("Blad blokowania semafora");
    }
}

void semaphore_signal(int semid)
{
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        handle_error("Blad odblokowania semafora");
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
            handle_error("Blad utworzenia semafora");
        }
    }
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        handle_error("Blad inicjalizacji semafora");
    }
    return semid;
}

void semaphore_remove(int semid)
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        handle_error("Blad usuwania semafora");
    }
}

void shared_memory_create(int *memory)
{
    *memory = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (*memory < 0)
    {
        handle_error("Blad utworzenia segmentu pamieci dzielonej");
    }
}

void shared_memory_address(int memory, Data **data)
{
    *data = (Data *)shmat(memory, NULL, 0);
    if (*data == (void *)-1)
    {
        handle_error("Blad dostepu do segmentu pamieci dzielonej");
    }
}

void shared_memory_detach(Data *data)
{
    if (shmdt(data) == -1)
    {
        handle_error("Blad odlaczenia pamieci dzielonej");
    }
}

void shared_memory_remove(int memory) 
{
    if (shmctl(memory, IPC_RMID, NULL) == -1) 
    {
        handle_error("Blad usuwania segmentu pamieci dzielonej");
    }
}

#endif // FUNCTIONS_H