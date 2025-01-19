#include "functions.h"

void handle_passenger(Data *data, int sem_passengers) // zarzadzanie pasazerami
{
    printf("KIEROWNIK POCIAGU: Oczekiwanie na pociag: %d.\n", data->current_train);

    int train_full_reported = 0; // flaga informujaca, czy komunikat o pelnym pociagu zostal wyswietlony

    while (data->passengers_waiting == 0 && data->generating) // oczekiwanie na wygenerowanie pasazerow
    {
        sleep(1);
    }
    while (data->generating != -1) // flaga na sprawdzenie zakonczenia dzialania zarzadcy
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            semaphore_wait(sem_passengers); // zablokowanie dostepu do danych pasazerow

            if (data->passengers_waiting < 0 || data->passengers_with_bikes < 0 || data->passengers_with_bikes > data->passengers_waiting)
                {
                    fprintf(stderr, "KIEROWNIK POCIAGU: Nieprawidlowe dane: liczba oczekujacych pasazerow: %d, z rowerami: %d\n",
                    data->passengers_waiting, data->passengers_with_bikes);
                    exit(EXIT_FAILURE);
                }

            if (data->passengers_with_bikes > 0)
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                data->passengers_with_bikes--;
                data->free_bike_spots--;
                printf("KIEROWNIK POCIAGU: Pasazer z rowerem wsiadl do pociagu %d i zajal miejsce %d.\n", data->current_train +1, seat +1);
            }
            else if (data->passengers_waiting > 0)
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                printf("KIEROWNIK POCIAGU: Pasazer bez roweru wsiadl do pociagu %d i zajal miejsce %d.\n", data->current_train +1, seat +1);
            }
            semaphore_signal(sem_passengers); // odblokowanie semafora pasazerow
        }

        if (data->free_seat == MAX_PASSENGERS && !train_full_reported) // pociag palny lub jeszcze nie zostal wyswietlony
        {
            printf("KIEROWNIK POCIAGU: Pociag %d jest gotowy do odjazdu.\n", data->current_train);
            train_full_reported = 1; // ustawienie flagi, aby komunikat nie pojawiaa sie ponownie
        }

        if (data->free_seat < MAX_PASSENGERS) // reset flagi przy przejsciu do kolejnego pociagu
        {
            train_full_reported = 0;
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