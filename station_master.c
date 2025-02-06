#include "functions.h"
#include "station_master.h"
#include "signal.h"

extern volatile sig_atomic_t running;

void handle_sigint(int sig)
{
    printf(COLOR_RED "ZARZADCA: Wyslano sygnal SIGINT.\n" COLOR_RESET);
    running = 0;
}

int main()
{
    int memory;
    Data *data = NULL;

    size_t dir_size = calculate_directory_size("."); // obliczanie rozmiaru katalogu
    size_t shared_memory_size = sizeof(Data); // obliczanie rozmiaru pamieci dzielonej

    if (dir_size == 0) // obsluga bledu rozmiaru katalogu
    {
        fprintf(stderr, COLOR_RED "ZARZADCA: Nie mozna obliczyc rozmiaru katalogu.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    if (shared_memory_size > dir_size / 10) // sprawdzenie czy pamiec dzielona nie przekracza limitu
    {
        fprintf(stderr, COLOR_RED "ZARZADCA: Rozmiar pamieci dzielonej (%zu B) przekracza 10%% rozmiaru katalogu (%zu B).\n" COLOR_RESET, shared_memory_size, dir_size);
        exit(EXIT_FAILURE);
    }

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint); // Correctly handling SIGINT for Ctrl+C

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    if (semctl(sem_train_entry, 0, SETVAL, 1) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla wejsc do pociagu");
    }
    if (semctl(sem_passengers_bikes, 0, SETVAL, 1) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora wejscia dla pasazerow z rowerami");
    }
    if (semctl(sem_passengers, 0, SETVAL, 1) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora wejscia dla pasazerow");
    }

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery
    data->passengers_waiting = 0; // ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

    semaphore_signal(sem_passengers_bikes); // odblokowanie aby uniknac konfliktu w synchronizacji
    semaphore_signal(sem_passengers); // odblokowanie aby uniknac konfliktu w synchronizacji

    pid_t train_manager_pid[MAX_TRAINS]; // tablica dla PIDow procesow kierownikow pociągow
    int i;
    for (i = 0; i < MAX_TRAINS; i++) 
    {
        train_manager_pid[i] = fork();
        if (train_manager_pid[i] < 0)
        {
            handle_error("ZARZADCA: Blad fork dla train_manager");
        }
        if (train_manager_pid[i] == 0) // proces potomny
        {
            char train_id_str[10];
            snprintf(train_id_str, sizeof(train_id_str), "%d", i); // konwersja ID pociagu do stringa
            if (execl("./train_manager", "./train_manager", train_id_str, NULL) == -1)
            {
                handle_error("ZARZADCA: Blad execl pliku train_manager");
            }
        }
    }

    pid_t passenger_pid[MAX_PASSENGERS_GENERATE];
    i = 0;
    data->generating = 1;

    while (i < MAX_PASSENGERS_GENERATE && running)
    {
        passenger_pid[i] = fork();

        if (passenger_pid[i] < 0)
        {
            handle_error("ZARZADCA: Blad fork dla passenger");
        }
        if (passenger_pid[i] == 0)
        {
            if (execl("./passenger", "./passenger", NULL) == -1)
            {
                handle_error("ZARZADCA: Blad execl pliku passenger");
            }
        }
        usleep(BLOCK_SLEEP * 100000);
        i++;
    }
    data->generating = 0;

    station_master(data, sem_passengers_bikes, sem_passengers, sem_train_entry);

    for (i = 0; i < MAX_TRAINS; i++) 
    {
        wait_for_child_process(train_manager_pid[i], "train_manager");
    }

    for (i = 0; i < MAX_PASSENGERS_GENERATE; i++) 
    {
        wait_for_child_process(passenger_pid[i], "passenger");
    }

    printf(COLOR_BLUE "ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n" COLOR_RESET);

    shared_memory_detach(data);
    shared_memory_remove(memory);
    semaphore_remove(sem_passengers_bikes);
    semaphore_remove(sem_passengers);
    semaphore_remove(sem_train_entry);

    return 0;
}