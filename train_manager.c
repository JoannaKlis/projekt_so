#include "functions.h"

int sem_passengers;

void handle_passenger(Data *data, int sem_passengers) // zarzadzanie pasazerami
{
    while (1)
    {
        semaphore_wait(sem_passengers);

        if (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0) // sprawdzenie czy pociag jest na stacji i czy jest gotowy na przyjecie pasazerow
        {
            if (data->passengers_with_bikes > 0 && data->free_bike_spots > 0)
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                data->passengers_with_bikes--;
                data->free_bike_spots--;
                printf("KIEROWNIK POCIAGU: Pasazer z rowerem wsiadl do pociagu %d i zajal miejsce %d.\n", data->current_train, seat);
            }
            else if (data->passengers_waiting > 0)
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                printf("KIEROWNIK POCIAGU: Pasazer bez roweru wsiadl do pociagu %d i zajal miejsce %d.\n", data->current_train, seat);
            }
        }

        if (data->free_seat == MAX_PASSENGERS || data->generating == -1)
        {
            semaphore_signal(sem_passengers);
            sleep(1); // oczekiwanie na nowy pociag
            continue;
        }

        semaphore_signal(sem_passengers);
        sleep(1);
    }

}

int main()
{
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);
    sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    handle_passenger(data, sem_passengers);

    shared_memory_detach(data);
    semaphore_remove(sem_passengers);

    return 0;
}