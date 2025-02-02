#ifndef PASSENGER_H
#define PASSENGER_H
#include "signal.h"

void passenger_process(Data *data, int sem_passengers_entry, int sem_bike_entry) 
{
    int has_bike = rand() % 2; // losowo decydujemy, czy pasazer ma rower
    data->passengers_waiting++;

    if (has_bike) 
    {
        semaphore_wait(sem_bike_entry); // czeka na dostep do wejscia dla pasazerow z rowerami
        int seat = data->free_seat; // id wolnego miejsca w pociagu
        data->train_data[data->current_train][seat] = getpid(); // przypisanie pasazera do miejsca
        data->free_seat++; // zwiekszenie liczby zajetych miejsc
        data->passengers_waiting--; // zmniejszenie liczby oczekujacych pasazerow
        data->free_bike_spots--; // zmniejszenie liczby wolnych miejsc na rowery
        printf(COLOR_MAGENTA "PASAZER: Pasazer %d z rowerem zajal miejsce %d w pociagu\n" COLOR_RESET, getpid(), seat + 1);
        if (data->free_bike_spots > 0 && data->free_seat < MAX_PASSENGERS) semaphore_signal(sem_bike_entry);
    } else 
    {
        semaphore_wait(sem_passengers_entry); // czeka na dostep do wejscia dla pasazerow
        int seat = data->free_seat; // id wolnego miejsca w pociagu
        data->train_data[data->current_train][seat] = getpid(); // przypisanie pasazera do miejsca
        data->free_seat++; // zwiekszenie liczby zajetych miejsc
        data->passengers_waiting--; // zmniejszenie liczby oczekujacych pasazerow
        printf(COLOR_MAGENTA "PASAZER: Pasazer %d zajal miejsce %d w pociagu.\n" COLOR_RESET, getpid(), seat + 1);
        if (data->free_seat < MAX_PASSENGERS) semaphore_signal(sem_passengers_entry);
    }

    pause(); // proces czeka na sygnal (SIGTERM) od zarzadcy pociagu
    printf(COLOR_PINK "PASAZER: Pasazer %d zakonczyl podroz.\n" COLOR_RESET, getpid());
    exit(0); // proces pasazera konczy swoje dzialanie
}

#endif // PASSENGER_H