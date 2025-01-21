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

// unikalne klucze IPC
#define SHM_KEY 0x1A2B3C4D // klucz pamieci wspoldzielonej
#define SEM_KEY_PASSENGERS 0x2B3C4D5E // klucz semafora pasazerow
#define SEM_KEY_PASSENGERS_BIKES 0x2B314D5E // klucz semafora pasazerow

// limity i konfiguracje systemu
#define MAX_PASSENGERS 20 // maksymalna liczba pasazerow w pociagu
#define MAX_BIKES 5 // maksymalna liczba rowerow w pociagu
#define MAX_TRAINS 4 // liczba pociagow
#define TRAIN_DEPARTURE_TIME 12 // czas odjazdu pociagu (w sekundach)
#define TRAIN_ARRIVAL_TIME 4 // czas przyjazdu pociagu na nastepna stacje (w sekundach)
#define BLOCK_SLEEP 1 // szybki dostep do wylaczenia funkcji sleep() - 0 aby wylaczyc

// kolorowanie terminala
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_ORANGE "\033[38;2;255;165;0m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_PINK "\033[38;5;213m"

typedef struct // struktura przechowujaca dane wspoldzielone
{
    short current_train; // aktualny pociag na stacji
    short free_seat; // numer 1 wolnego miejsca w pociagu
    short train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    short passengers_waiting; // liczba oczekujacych pasazerow
    short generating; // flaga czy pasazerowie sa generowani
    short free_bike_spots; // liczba wolnych miejsc na rowery
    short passengers_with_bikes; // pasazerowie z rowerami
} Data;

extern int sem_passengers; // zmienna globalna dla semaforow

int run_for_Ttime() // odjazd pociagu po T czasie
{
    time_t start_time = time(NULL);
    time_t current_time;
    while (1)
    {
        current_time = time(NULL);
        if (current_time - start_time >= TRAIN_DEPARTURE_TIME)
        {
            return 1;
        }
    }
}

void handle_error(const char *message) // funkcja obslugi bledow
{
    fprintf(stderr, COLOR_RED "%s" COLOR_RESET "\n", message);
    perror("");
    exit(EXIT_FAILURE);
}

size_t calculate_directory_size(const char *path) // obliczanie calkowitego rozmiaru katalogu
{
    size_t total_size = 0;
    struct stat st; // struktura przechowujaca informacje o pliku
    struct dirent *entry; // struktura opisujaca wpis w katlogu

    DIR *dir = opendir(path); // otworzenie katalogu
    if (!dir)
    {
        fprintf(stderr, COLOR_RED "Blad otwierania katalogu" COLOR_RESET "\n");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) // iteracja przez wszystkie pliki w katalogu
    {
        char full_path[PATH_MAX]; // sciezka do pliku
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &st) == 0) // pobranie informacji o pliku
        {
            if (S_ISREG(st.st_mode)) // tylko pliki regularne sa liczone
                total_size += st.st_size; // dodanie rozmiaru pliku do calkowitego rozmiaru
        }
    }

    closedir(dir);
    return total_size;
}

void semaphore_wait(int semid)
{
    //printf("DEBUG: Wartość semafora PRZED WAIT: %d\n", semctl(semid, 0, GETVAL));
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        handle_error("Blad blokowania semafora");
    }
    //printf("DEBUG: Wartość semafora PO WAIT: %d\n", semctl(semid, 0, GETVAL));
}

void semaphore_signal(int semid)
{
    //printf("DEBUG: Wartość semafora PRZED SIGNAL: %d\n", semctl(semid, 0, GETVAL));
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        handle_error("Blad odblokowania semafora");
    }
    //printf("DEBUG: Wartość semafora PO SIGNAL: %d\n", semctl(semid, 0, GETVAL));
}

int semaphore_create(key_t key) // funkcja tworzaca semafor
{
    int semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600); // tworzenie nowego semafora
    if (semid == -1)
    {
        if (errno == EEXIST) // jesli juz istnieje to otwiera go
        {
            semid = semget(key, 1, 0600);
        }
        else
        {
            handle_error("Blad utworzenia semafora");
        }
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) // inicjalizacja semafora wartoscia 1
    {
        handle_error("Blad inicjalizacji semafora");
    }
    return semid; // zwrocenie id semafora
}

void semaphore_remove(int semid) // funkcja usuwania semafora
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        handle_error("Blad usuwania semafora");
    }
}

void shared_memory_create(int *memory) // funkcja tworzaca segment pamieci wspoldzielonej
{
    *memory = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (*memory < 0)
    {
        handle_error("Blad utworzenia segmentu pamieci dzielonej");
    }
}

void shared_memory_address(int memory, Data **data) // funkcja uzyskujaca adres segmentu pamieci wspoldzielonej
{
    *data = (Data *)shmat(memory, NULL, 0);
    if (*data == (void *)-1)
    {
        handle_error("Blad dostepu do segmentu pamieci dzielonej");
    }
}

void shared_memory_detach(Data *data) // funkcja odlaczajaca segment pamieci wspoldzielonej
{
    if (shmdt(data) == -1)
    {
        handle_error("Blad odlaczenia pamieci dzielonej");
    }
}

void shared_memory_remove(int memory) // funkcja usuwajaca segment pamieci wspoldzielonej
{
    if (shmctl(memory, IPC_RMID, NULL) == -1)
    {
        handle_error("Blad usuwania segmentu pamieci dzielonej");
    }
}

#endif // FUNCTIONS_H