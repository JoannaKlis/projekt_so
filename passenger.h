#ifndef PASSENGER_H
#define PASSENGER_H
#include "signal.h"

void passengers_generating(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) // generowanie pasazerow i aktualizacja danych wspoldzielonych
{
    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        int has_bike = rand() % 2; // przypisanie czy pasazer ma rower czy nie

        if (has_bike)
        {
            semaphore_wait(sem_passengers_bikes); // czekanie na dostep do wejscia dla pasazerow z rowerami
            if (data->free_bike_spots > 0)
            {
                semaphore_wait(sem_train_entry); // czekanie na pozwolenie na wejscie
                data->passengers_waiting++;
                data->passengers_with_bikes++;
                data->free_bike_spots--;

                printf(COLOR_MAGENTA "PASAZER: Na stacji pojawil sie pasazer z rowerem.\n" COLOR_RESET);
                semaphore_signal(sem_train_entry); // zwolnienie miejsca na kolejnego pasazera
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

                printf(COLOR_MAGENTA "PASAZER: Na stacji pojawil sie pasazer.\n" COLOR_RESET);
                semaphore_signal(sem_train_entry); // zwolnienie miejsca na kolejnego pasazera

            }
            semaphore_signal(sem_passengers); // zwolnienie semafora dla pasazerow
        }
        sleep(BLOCK_SLEEP*1);

        //printf(COLOR_MAGENTA "PASAZER: Oczekiwanie na informacje o pasazerach bez rowerow.\n" COLOR_RESET);
        //semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych
        //printf(COLOR_GREEN "PASAZER: Otrzymano informacje o pasazerach bez rowerow.\n" COLOR_RESET);

        //if (data->free_seat == MAX_PASSENGERS) // sprawdzanie, czy miejsca w pociagu sa dostepne
        //{
        //    semaphore_signal(sem_passengers); // blokada wsiadania gdy nie ma wolnych miejsc
        //    printf(COLOR_RED "PASAZER: Brak wolnych miejsc w pociagu.\n" COLOR_RESET);
        //    sleep(BLOCK_SLEEP*1);
        //    continue;
        //}

        //int passengers_without_bikes = 5 + rand() % 6; // losowa liczba pasazerow bez rowerow (5-10)
        //data->passengers_waiting += passengers_without_bikes; // zwiekszenie liczby oczekujacych pasazerow

        //printf(COLOR_MAGENTA "PASAZER: Pojawilo sie %d nowych pasazerow bez rowerow.\n" COLOR_RESET, passengers_without_bikes);
        //semaphore_signal(sem_passengers); // odblokowanie danych wspoldzielonych
        //printf(COLOR_RED "PASAZER: Przekazanie informacji ile pojawilo sie nowych pasazerow bez rowerow.\n" COLOR_RESET);

        //printf(COLOR_MAGENTA "PASAZER: Oczekiwanie na informacje o pasazerach z rowerami.\n" COLOR_RESET);
        //semaphore_wait(sem_passengers_bikes);
        //printf(COLOR_GREEN "PASAZER: Otrzymano informacje o pasazerach bez rowerow.\n" COLOR_RESET);

        //if (data->free_bike_spots > 0)
        //{
        //    int passengers_with_bikes = 4 + rand() % 5; // losowa liczba pasażerów z rowerami (4-8)
        //    data->passengers_waiting += passengers_with_bikes;
        //    data->passengers_with_bikes += passengers_with_bikes;
        //    data->free_bike_spots -= passengers_with_bikes; // aktualizacja wolnych miejsc na rowery
//
        //    printf(COLOR_MAGENTA "PASAZER: Pojawilo sie %d nowych pasazerow z rowerami.\n" COLOR_RESET, passengers_with_bikes);
        //}
        //else
        //{
        //    printf(COLOR_MAGENTA "PASAZER: Brak miejsc dla pasazerow z rowerami.\n" COLOR_RESET);
        //}

        //semaphore_signal(sem_passengers_bikes);
        //printf(COLOR_RED "PASAZER: Zablokowano wejscie dla pasazerow z rowerami.\n" COLOR_RESET);

        //sleep(BLOCK_SLEEP*1);
    }
    data->generating = 0; // flaga generowanie zakonczone
}

#endif // PASSENGER_H