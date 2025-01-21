#include "functions.h"

void wait_for_child_process(pid_t pid, const char *process_name) // oczekiwanie na zakonczenie procesu potomnego
{
    printf(COLOR_CYAN "ZARZADCA: Oczekiwanie na zakonczenie procesu %s (PID: %d).\n" COLOR_RESET, process_name, pid);

    if (waitpid(pid, NULL, 0) == -1) // oczekiwanie na zakonczenie procesu za pomoca waitpid
    {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), COLOR_RED "ZARZADCA: Blad podczas oczekiwania na %s" COLOR_RESET, process_name);
        handle_error(error_message);
    }
    printf(COLOR_GREEN "ZARZADCA: Proces %s (PID: %d) zakonczony pomyslnie.\n" COLOR_RESET, process_name, pid);
}

void station_master(Data *data, int sem_passengers_bikes, int sem_passengers) // zarzadzanie pociagami
{
    while (1)
    {
        printf(COLOR_CYAN "ZARZADCA: Pociag %d przyjechal na stacje 1.\n" COLOR_RESET, data->current_train + 1);

        semaphore_signal(sem_passengers_bikes); // odblokowanie wejscia do pociagu dla pasazerow z rowerami
        printf(COLOR_GREEN "ZARZADCA: Wejscie dla pasazerow z rowerami odblokowane.\n" COLOR_RESET);

        semaphore_signal(sem_passengers); // odblokowanie wejscia do pociagu dla pasazera bez rowerow
        printf(COLOR_GREEN "ZARZADCA: Wejscie dla pasazerow bez rowerow odblokowane.\n" COLOR_RESET);

        printf(COLOR_CYAN "ZARZADCA: Liczba wszystkich oczekujacych: %d.\n" COLOR_RESET, data->passengers_waiting);
        sleep(WAITING_FOR_PASSENGERS_TO_BOARD); // oczekiwanie na wejscie pasazerow

        if (run_for_Ttime())
        {
            printf(COLOR_CYAN "ZARZADCA: Pociag %d odjezdza ze stacji 1.\n" COLOR_RESET, data->current_train + 1);

            semaphore_wait(sem_passengers_bikes); // blokada wejscia do pociagu dla pasazerow z rowerami
            printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow z rowerami zablokowane.\n" COLOR_RESET);

            semaphore_wait(sem_passengers); // blokada wejscia do pociagu dla pasazerow bez rowerami
            printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow bez rowerami zablokowane.\n" COLOR_RESET);

            data->free_seat = MAX_PASSENGERS; // aktualizacja liczby miejsc w pociagu

            printf(COLOR_CYAN "ZARZADCA: Pociag %d dotarl na stacje 2.\n" COLOR_RESET, data->current_train + 1);

            sleep(TRAIN_ARRIVAL_TIME); // prowrot ze stacji 2

            data->free_seat = 0; // reset miejsc po powrocie
            data->free_bike_spots = MAX_BIKES; // reset miejsc dla rowerow

            printf(COLOR_CYAN "ZARZADCA: Pociag %d wrocil na stacje 1.\n" COLOR_RESET, data->current_train + 1);

            data->current_train = (data->current_train + 1) % MAX_TRAINS; // obieg z pociagami
        }

        if (data->passengers_waiting == 0 && !data->generating) // sprawdzenie, czy brak oczekujacych pasazerow oraz czy zakonczono generowanie
        {
            printf(COLOR_BLUE "ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n" COLOR_RESET);
            data->generating = -1; // flaga - zarzadca konczy dzialanie

            // odblokowanie semaforow na zakonczenie dzialania
            semaphore_signal(sem_passengers);
            semaphore_signal(sem_passengers_bikes);
            break;
        }
    }
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

    pid_t passenger_pid = fork(); // uruchomienie procesu generowania pasazerow
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

    station_master(data, sem_passengers_bikes, sem_passengers); // uruchomienie dzialania zarzadcy stacji

    // oczekiwanie na zakonczenie procesow potomnych
    wait_for_child_process(passenger_pid, "passenger");
    wait_for_child_process(train_manager_pid, "train_manager");

    shared_memory_detach(data); // zwolnienie pamieci dzielonej
    shared_memory_remove(memory); // zwolnienie semaforow

    return 0;
}