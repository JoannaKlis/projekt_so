#ifndef PASSENGER_H
#define PASSENGER_H
#include "signal.h"

void passengers_generating(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) // generowanie jednego pasazera
{
    if (!running) 
    {
        printf(COLOR_RED "PASAZER: Generowanie pasazerow zakonczone.\n" COLOR_RESET);
        data->generating = 0; // generowanie na zakonczone
        return;
    }

    int has_bike = rand() % 2; // przypisanie czy pasazer ma rower czy nie

    if (has_bike)
    {
        semaphore_wait(sem_passengers_bikes); // czekanie na dostep do wejscia dla pasaserow z rowerami
        if (data->free_bike_spots > 0)
        {
            semaphore_wait(sem_train_entry); // czekanie na pozwolenie na wejscie
            data->passengers_waiting++;
            data->passengers_with_bikes++;
            data->free_bike_spots--;
            data->passenger_pid = getpid();

            printf(COLOR_MAGENTA "PASAZER: Na stacji pojawi sie nowy pasazer %d z rowerem.\n" COLOR_RESET, data->passenger_pid);
            semaphore_signal(sem_train_entry); // zwolnienie miejsca na kolejnego pasasera
        }
        semaphore_signal(sem_passengers_bikes); // zwolnienie semafora dla rowerow
    }
    else
    {
        semaphore_wait(sem_passengers);

        if (data->free_seat < MAX_PASSENGERS)
        {
            semaphore_wait(sem_train_entry); // czekanie na pozwolenie na wejscie
            data->passengers_waiting++;
            data->passenger_pid = getpid();

            printf(COLOR_MAGENTA "PASAZER: Na stacji pojawi sie nowy pasazer %d.\n" COLOR_RESET, data->passenger_pid);
            semaphore_signal(sem_train_entry); // zwolnienie miejsca na kolejnego pasazera
        }
        semaphore_signal(sem_passengers); // zwolnienie semafora dla pasaserow
    }
    kill(getpid(), SIGTERM);
}

#endif // PASSENGER_H