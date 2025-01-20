#include "functions.h"

void wait_for_child_process(pid_t pid, const char *process_name) // oczekiwanie na zakonczenie procesu potomnego
{
    if (waitpid(pid, NULL, 0) == -1)
    {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), COLOR_RED "ZARZADCA: Blad podczas oczekiwania na %s" COLOR_RESET, process_name);
        handle_error(error_message);
    }
}

void station_master(Data *data, int sem_passengers, int sem_train_departure) // zarzadzanie odjazdami pociagow
{
    while (1)  // dzialanie zarzadcy
    {
        semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych

        if (data->passengers_waiting == 0 && !data->generating) // sprawdzenie, czy brak oczekujacych pasazerow oraz czy zakonczono generowanie
        {
            printf(COLOR_BLUE "ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n" COLOR_RESET);
            data->generating = -1; // flaga - zarzadca konczy dzialanie
            semaphore_signal(sem_passengers);
            break;
        }

        if (data->passengers_waiting > 0) // gdy sa oczekujacy pasazerowie
        {
            printf(COLOR_CYAN "ZARZADCA: Pociag %d przyjechal na stacje 1.\n" COLOR_RESET, data->current_train + 1);
            printf(COLOR_CYAN "ZARZADCA: Liczba wszystkich oczekujacych: %d.\n" COLOR_RESET, data->passengers_waiting);

            if (run_for_Ttime()) // odjazd pociagu po T czasie
            {
                semaphore_signal_train_departure(sem_train_departure); // odblokowanie semafora po uplywie czasu
            }

            printf(COLOR_CYAN "ZARZADCA: Pociag %d odjezdza ze stacji 1.\n" COLOR_RESET, data->current_train + 1);

            data->free_seat = MAX_PASSENGERS; // oznaczenie pociagu jako pelny
            //sleep(TRAIN_ARRIVAL_TIME); // podroz na stacje 2

            printf(COLOR_CYAN "ZARZADCA: Pociag %d dotarl na stacje 2.\n" COLOR_RESET, data->current_train + 1);
            printf(COLOR_CYAN "ZARZADCA: Pasazerowie wysiadaja na stacji 2.\n" COLOR_RESET);

            //sleep(1); // czas wysiadania

            data->free_seat = 0; // reset wolnych miejsc
            data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery

            printf(COLOR_CYAN "ZARZADCA: Pociag %d wrocil na stacje 1.\n" COLOR_RESET, data->current_train + 1);

            data->current_train = (data->current_train + 1) % MAX_TRAINS; // nastepny pociag
        }
        semaphore_signal(sem_passengers); // zwolnienie semafora
    }
    semaphore_remove(sem_train_departure); // usuniecie semafora odjazdu pociagu po zakonczeniu dzialania
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

    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);
    int sem_train_departure = semaphore_create(SEM_KEY_TRAIN_DEPARTURE);

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery
    data->passengers_waiting = 0; // ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

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

    station_master(data, sem_passengers, sem_train_departure);

    wait_for_child_process(train_manager_pid, "train_manager");
    wait_for_child_process(passenger_pid, "passenger");

    shared_memory_detach(data);
    shared_memory_remove(memory);

    return 0;
}