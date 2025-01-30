#include "functions.h"
#include "station_master.h"
#include "signal.h"

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

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    if (semctl(sem_passengers_bikes, 0, SETVAL, 1) == -1)
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla pasazerow z rowerami");
    }
    if (semctl(sem_passengers, 0, SETVAL, 1) == -1)
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla pasazerow bez rowerow");
    }

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery
    data->passengers_waiting = 0; // ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

    semaphore_signal(sem_passengers_bikes); // odblokowanie aby uniknac konfliktu w synchronizacji
    semaphore_signal(sem_passengers); // odblokowanie aby uniknac konfliktu w synchronizacji

    pid_t train_manager_pid = fork(); // uruchomienie procesu kierownika pociagu
    if (train_manager_pid < 0) // blad w fork
    {
        handle_error("ZARZADCA: Blad fork dla train_manager");
    }
    if (train_manager_pid == 0) // kod w procesie potomnym
    {
        if (execl("./train_manager", "./train_manager", NULL) == -1) // sprawdzenie bledu execl
        {
            handle_error("ZARZADCA: Blad execl pliku train_manager");
        }
    }

    pid_t passenger_pid; // deklaracja pidu pasazera

    int i = 0;
    data->generating = 1; // flaga - generowanie rozpoczete

    while (i < MAX_PASSENGERS_GENERATE && running)
    {
        passenger_pid = fork();

        if (passenger_pid < 0) // blad w fork
        {
            handle_error("ZARZADCA: Blad fork dla passenger");
        }
        if (passenger_pid == 0) // kod w procesie potomnym
        {
            if (execl("./passenger", "./passenger", NULL) == -1) // sprawdzenie bledu execl
            {
                handle_error("ZARZADCA: Blad execl pliku passenger");
            }
        }
        usleep(BLOCK_SLEEP * 100000);
        i++;
    }
    data->generating = 0; // Zakończenie generowania

    station_master(data, sem_passengers_bikes, sem_passengers); // uruchomienie dzialania zarzadcy stacji

    // oczekiwanie na zakonczenie procesow potomnych
    wait_for_child_process(passenger_pid, "passenger");
    wait_for_child_process(train_manager_pid, "train_manager");

    shared_memory_detach(data); // zwolnienie pamieci dzielonej
    shared_memory_remove(memory); // zwolnienie semaforow

    return 0;
}