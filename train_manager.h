#ifndef TRAIN_MANAGER_H
#define TRAIN_MANAGER_H
#include "signal.h"

void handle_passenger(Data *data, int sem_passengers_entry, int sem_bike_entry) // zarzadzanie pasazerami
{
    semaphore_set_to_one(sem_bike_entry); // odblokowanie wejscia do pociagu dla pasazerow z rowerami
    printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Wejscie dla pasazerow z rowerami odblokowane.\n" COLOR_RESET);

    semaphore_set_to_one(sem_passengers_entry); // odblokowanie wejscia do pociagu dla pasazera bez rowerow
    printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Wejscie dla pasazerow bez rowerow odblokowane.\n" COLOR_RESET);

    data->free_seat = 0; // reset miejsc po powrocie
    data->free_bike_spots = MAX_BIKES; // reset miejsc dla rowerow
    int train_full_reported = 0; // flaga informujaca, czy komunikat o pelnym pociagu zostal wyswietlony

    while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0) // wsiadanie do pociagu
    {
    if (data->free_bike_spots <= 0) // obsluga pasazerow z rowerami
        {
            semaphore_set_to_zero(sem_bike_entry);
        }
    }
    semaphore_set_to_zero(sem_passengers_entry);

    if (data->free_seat == MAX_PASSENGERS && !train_full_reported) // pociag jest pelny
    {
        printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d jest gotowy do odjazdu.\n" COLOR_RESET, data->current_train + 1);
        train_full_reported = 1;
    }

    if (data->free_seat < MAX_PASSENGERS) // reset flagi przy przejsciu do kolejnego pociagu
    {
        train_full_reported = 0;
    }

    if (data->generating == -1) // czeka az zarzadca skonczy rozwozic pasazerow
    {
        printf(COLOR_ORANGE "KIEROWNIK POCIAGU: Brak oczekujacych pasazerow\nKONIEC DZIALANIA.\n" COLOR_RESET);
        exit(0);
    }
}

#endif // TRAIN_MANAGER_H