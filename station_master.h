#ifndef STATION_MASTER_H
#define STATION_MASTER_H
#include "signal.h"

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

void station_master(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) // zarzadzanie pociagami
{
    while (1)
    {
        printf(COLOR_CYAN "ZARZADCA: Pociag %d przyjechal na stacje 1.\n" COLOR_RESET, data->current_train + 1);

        semaphore_signal(sem_passengers_bikes); // odblokowanie wejscia do pociagu dla pasazerow z rowerami
        printf(COLOR_GREEN "ZARZADCA: Wejscie dla pasazerow z rowerami odblokowane.\n" COLOR_RESET);

        semaphore_signal(sem_passengers); // odblokowanie wejscia do pociagu dla pasazera bez rowerow
        printf(COLOR_GREEN "ZARZADCA: Wejscie dla pasazerow bez rowerow odblokowane.\n" COLOR_RESET);

        printf(COLOR_CYAN "ZARZADCA: Liczba wszystkich oczekujacych: %d.\n" COLOR_RESET, data->passengers_waiting);

        if (signal2 == 0)
        {
            printf(COLOR_YELLOW "ZARZADCA: Sygnał blokady wejścia pasażerów (CTRL+L).\n" COLOR_RESET);
            sleep(1); // tymczasowa blokada wejścia pasażerów na 1 sekundę
            signal2 = 1; // reset flagi signal2
        }

        sleep(BLOCK_SLEEP * 2); // oczekiwanie na wejscie pasazerow

        if (signal1 == 0)
        {
            printf(COLOR_YELLOW "ZARZADCA: Natychmiastowy odjazd pociągu wymuszony sygnałem (CTRL+K).\n" COLOR_RESET);
            signal1 = 1; // reset flagi signal1
        }
        else if (run_for_Ttime())
        {
            printf(COLOR_CYAN "ZARZADCA: Pociag %d odjezdza ze stacji 1.\n" COLOR_RESET, data->current_train + 1);
        }

        semaphore_wait(sem_passengers_bikes); // blokada wejscia do pociagu dla pasazerow z rowerami
        printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow z rowerami zablokowane.\n" COLOR_RESET);

        semaphore_wait(sem_passengers); // blokada wejscia do pociagu dla pasazerow bez rowerow
        printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow bez rowerow zablokowane.\n" COLOR_RESET);

        data->free_seat = MAX_PASSENGERS; // aktualizacja liczby miejsc w pociagu

        printf(COLOR_CYAN "ZARZADCA: Pociag %d dotarl na stacje 2.\n" COLOR_RESET, data->current_train + 1);

        sleep(BLOCK_SLEEP * TRAIN_ARRIVAL_TIME); // powrot ze stacji 2

        data->free_seat = 0; // reset miejsc po powrocie
        data->free_bike_spots = MAX_BIKES; // reset miejsc dla rowerow

        printf(COLOR_CYAN "ZARZADCA: Pociag %d wrocil na stacje 1.\n" COLOR_RESET, data->current_train + 1);

        data->current_train = (data->current_train + 1) % MAX_TRAINS; // obieg z pociagami

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

#endif // STATION_MASTER_H