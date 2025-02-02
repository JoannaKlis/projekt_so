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
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdarg.h>

// unikalne klucze IPC
#define SHM_KEY 0x1A2B3C4D // klucz pamieci wspoldzielonej
#define SEM_KEY_TRAIN_ENTRY 0x2B314D1E // klucz semafora pociagu
#define SEM_KEY_PASSENGERS_ENTRY 0x2B3C4D6F // semafor dla wejscia pasazerow
#define SEM_KEY_PASSENGERS_EXIT 0x2B3C4D7A // semafor dla wyjscia pasazerow
#define SEM_KEY_BIKE_ENTRY 0x2B3C4D8A // semafor dla wejscia pasazerow z rowerami
#define SEM_KEY_BIKE_EXIT 0x2B3C4D9A // semafor dla wyjscia pasazerow z rowerami

// limity i konfiguracje systemu
#define MAX_PASSENGERS_GENERATE 100 //maksymalna liczba dopuszczalnych procesow
#define MAX_PASSENGERS 20 // maksymalna liczba pasazerow w pociagu
#define MAX_BIKES 5 // maksymalna liczba rowerow w pociagu
#define MAX_TRAINS 4 // liczba pociagow
#define TRAIN_DEPARTURE_TIME 1 // czas odjazdu pociagu (w sekundach)
#define TRAIN_ARRIVAL_TIME 1 // czas przyjazdu pociagu na nastepna stacje (w sekundach)
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

#define MSG_KEY 0x2B3C4D7F // klucz dla kolejki komunikatów

struct TrainMessage 
{
    long mtype;    // typ wiadomosci (np. numer pociągu)
    int train_number; // numer pociągu
};


typedef struct // struktura przechowujaca dane wspoldzielone
{
    short current_train; // aktualny pociag na stacji
    short free_seat; // index wolnego miejsca w pociagu
    short train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    short passengers_waiting; // liczba oczekujacych pasazerow
    short generating; // flaga czy pasazerowie sa generowani
    short free_bike_spots; // liczba wolnych miejsc na rowery
    pid_t passenger_pid;
} Data;

//int run_for_Ttime() // odjazd pociagu po T czasie
//{
//    time_t start_time = time(NULL);
//    time_t current_time;
//    while (1)
//    {
//        current_time = time(NULL);
//        if (current_time - start_time >= TRAIN_DEPARTURE_TIME)
//        {
//            return 1;
//        }
//    }
//}

void handle_error(const char *message) // funkcja obslugi bledow
{
    fprintf(stderr, COLOR_RED "%s" COLOR_RESET "\n", message);
    perror("");
    exit(EXIT_FAILURE);
}

void semaphore_wait(int semid)
{
    //printf("DEBUG: Wartosc semafora PRZED WAIT: %d\n", semctl(semid, 0, GETVAL));
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        handle_error("Blad blokowania semafora");
    }
    //printf("DEBUG: Wartosc semafora PO WAIT: %d\n", semctl(semid, 0, GETVAL));
}

void semaphore_decrement(int sem_id) // obnizenie wartosci semafora bez czekania
{
    struct sembuf operation = {0, -1, IPC_NOWAIT};
    if (semop(sem_id, &operation, 1) == -1) 
    {
        if (errno != EAGAIN) // jesli nie mozna zmniejszyc wartosci, ale to nie brak zasobow
        {
            handle_error("Blad zmniejszania wartosci semafora");
        }
    } else 
    {
        printf(COLOR_GREEN "Semafor zmniejszony bez oczekiwania.\n" COLOR_RESET);
    }
}

void semaphore_set_to_one(int sem_id) // ustawienie wartosci semafora na 1
{
    if (semctl(sem_id, 0, SETVAL, 1) == -1) 
    {
        handle_error("ZARZADCA: Blad ustawiania wartosci semafora na 1");
    }
}

void semaphore_set_to_zero(int sem_id) // ustawienie wartosci semafora na 0
{
    if (semctl(sem_id, 0, SETVAL, 0) == -1) 
    {
        handle_error("ZARZADCA: Blad ustawiania wartosci semafora na 1");
    }
}


void semaphore_signal(int semid)
{
    //printf("DEBUG: Wartosc semafora PRZED SIGNAL: %d\n", semctl(semid, 0, GETVAL));
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        handle_error("Blad odblokowania semafora");
    }
    //printf("DEBUG: Wartosc semafora PO SIGNAL: %d\n", semctl(semid, 0, GETVAL));
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

// Odbiera pierwszy komunikat typu msgtype bez możliwości przerwania przez sygnał
int receive_message(int msq_ID, long msgtype, struct TrainMessage *msg) {
    while (1) {
        if (msgrcv(msq_ID, msg, sizeof(struct TrainMessage) - sizeof(long), msgtype, 0) == -1) {
            if (errno == EINTR) {
                printf("Przerwano przez sygnał podczas odbierania komunikatu o typie: %ld\n", msgtype);
                // Przerwane przez sygnał – ponawiamy
                continue;
            } else if (errno == ENOMSG) {
                // Brak komunikatu
                printf("Brak komunikatu w kolejce o typie: %ld\n", msgtype);
                continue;
            } else {
                // Błąd
                printf("Blad recieve_message: %ld\n", msgtype);
                return -1;
            }
        }
        // Sukces
        return 1;
    }
}

// Wysyła komunikat
int send_message(int msq_ID, struct TrainMessage *msg) {
    while (1) {
        if (msgsnd(msq_ID, msg, sizeof(struct TrainMessage) - sizeof(long), 0) == -1) {
            if (errno == EINTR) {
                printf("Przerwano przez sygnał podczas wysyłania komunikatu\n");
                // Przerwane przez sygnał – ponawiamy
                continue;
            }
            perror("Blad send_message");
            return -1;
        }
        // Sukces
        return 0;
    }
}

#endif // FUNCTIONS_H