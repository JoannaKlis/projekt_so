#include "functions.h"

void handle_passenger(Data *data, int sem_passengers) // zarzadzanie pasazerami
{
    printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Oczekiwanie na pociag: %d.\n" COLOR_RESET, data->current_train + 1);

    int train_full_reported = 0; // flaga informujaca, czy komunikat o pelnym pociagu zostal wyswietlony

    while (data->passengers_waiting == 0 && data->generating) // oczekiwanie na wygenerowanie pasazerow
    {
        //sleep(1);
    }
    while (data->generating != -1) // flaga na sprawdzenie zakonczenia dzialania zarzadcy
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0) // wsiadanie do pociagu
        {
            semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych

            //if (data->passengers_waiting < 0 || data->passengers_with_bikes < 0 || data->passengers_with_bikes > data->passengers_waiting) // walidacja spojnosci danych
            //{
               // fprintf(stderr, COLOR_RED "KIEROWNIK POCIAGU: Nieprawidlowe dane: liczba oczekujacych pasazerow: %d, z rowerami: %d\n" COLOR_RESET, data->passengers_waiting, data->passengers_with_bikes);
               // exit(EXIT_FAILURE); // koniec programu gdy dane sa nieprawidlowe
           // }

            if (data->passengers_with_bikes > 0 && data->free_bike_spots > 0 && data->passengers_with_bikes <= MAX_BIKES) // sprawdzamy, czy liczba pasazerow z rowerami nie przekroczy MAX_BIKES
            {
                int seat = data->free_seat; // id wolnego miejsca w pociagu
                data->train_data[data->current_train][seat] = getpid(); // przypisanie pasazera do miejsca
                data->free_seat++; // zwiekszenie liczby zajetych miejsc
                data->passengers_waiting--; // zmniejszenie liczby oczekujacych pasazerow
                data->passengers_with_bikes--; // zmniejszenie liczby pasazerow z rowerami
                data->free_bike_spots--; // zmniejszenie liczby wolnych miejsc na rowery
                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer z rowerem wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->current_train + 1, seat + 1);
            }
            else if (data->passengers_waiting > 0) // obsluga pasazerow bez rowerow
            {
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer bez roweru wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->current_train + 1, seat + 1);
            }

            semaphore_signal(sem_passengers); // odblokowanie dostepu do danych wspoldzielonych
        }

        if (data->free_seat == MAX_PASSENGERS && !train_full_reported) // pociag pelny
        {
            printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d jest gotowy do odjazdu.\n" COLOR_RESET, data->current_train +1);
            train_full_reported = 1; // ustawienie flagi
        }

        if (data->free_seat < MAX_PASSENGERS) // reset flagi przy przejsciu do kolejnego pociagu
        {
            train_full_reported = 0;
        }
        //sleep(1);
    }

    printf(COLOR_GREEN "KIEROWNIK POCIAGU: Brak oczekujacych pasazerow\nKONIEC DZIALANIA.\n" COLOR_RESET);
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