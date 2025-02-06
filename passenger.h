#ifndef PASSENGER_H
#define PASSENGER_H
#include "signal.h"

void passengers_generating(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) 
{
    int has_bike = rand() % 2; 

    if (has_bike)
    {
        semaphore_wait(sem_passengers_bikes); 
        if (data->free_bike_spots > 0)
        {
            semaphore_wait(sem_train_entry); 
            data->passengers_waiting++;
            data->passengers_with_bikes++;
            data->free_bike_spots--;
            data->passenger_pid = getpid();

            for (int i = 0; i < MAX_PASSENGERS_GENERATE; i++) 
            {
                if (data->passenger_pids[i] == 0) 
                {
                    data->passenger_pids[i] = data->passenger_pid;
                    break;
                }
            }

            printf(COLOR_MAGENTA "PASAZER: Na stacji pojawi sie nowy pasazer %d z rowerem.\n" COLOR_RESET, data->passenger_pid);
            semaphore_signal(sem_train_entry); 
        }
        semaphore_signal(sem_passengers_bikes); 
    }
    else
    {
        semaphore_wait(sem_passengers);

        if (data->free_seat < MAX_PASSENGERS)
        {
            semaphore_wait(sem_train_entry); 
            data->passengers_waiting++;
            data->passenger_pid = getpid();

            for (int i = 0; i < MAX_PASSENGERS_GENERATE; i++) 
            {
                if (data->passenger_pids[i] == 0) 
                {
                    data->passenger_pids[i] = data->passenger_pid;
                    break;
                }
            }

            printf(COLOR_MAGENTA "PASAZER: Na stacji pojawi sie nowy pasazer %d.\n" COLOR_RESET, data->passenger_pid);
            semaphore_signal(sem_train_entry); 
        }
        semaphore_signal(sem_passengers); 
    }
}

#endif // PASSENGER_H