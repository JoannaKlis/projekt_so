#include "functions.h"

void handle_passenger(Data *data, int sem_passengers) // zarzadzanie pasazerami
{
    printf("KIEROWNIK POCIAGU: Oczekiwanie na pociag: %d.\n", data->current_train);

    while (data->passengers_waiting == 0 && data->generating) // oczekiwanie na wygenerowanie pasazerow
    {
        sleep(1);
    }
    printf("KIEROWNIK POCIAGU: Rozpoczynam zarzadzanie pasazerami dla pociagu %d.\n", data->current_train);

    while (data->generating != -1) // flaga na sprawdzenie zakonczenia dzialania zarzadcy
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            semaphore_wait(sem_passengers); // zablokowanie dostepu do danych pasazerow

            if (data->passengers_waiting < 0 || data->passengers_with_bikes < 0) 
            {
                fprintf(stderr, "KIEROWNIK POCIAGU: Nieprawidlowy stan danych!\n");
                exit(EXIT_FAILURE);
            }

            if (data->passengers_with_bikes > 0)
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid(); // PID pasazera
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
            semaphore_signal(sem_passengers); // odblokowanie semafora pasazerow
        }

        if (data->free_seat == MAX_PASSENGERS)
        {
            printf("KIEROWNIK POCIAGU: Pociag %d jest pelny i gotowy do odjazdu.\n", data->current_train);
        }
        sleep(1);
    }
    printf("KIEROWNIK POCIAGU: Brak oczekujacych pasazerow\nKONIEC DZIALANIA.\n");
}

int main()
{
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);
    
    handle_passenger(data, sem_passengers);

    shared_memory_detach(data);
    semaphore_remove(sem_passengers);
    
    return 0;
}