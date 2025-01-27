#ifndef TRAIN_MANAGER_H
#define TRAIN_MANAGER_H

void handle_passenger(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) // zarzadzanie pasazerami
{
    printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Oczekiwanie na pociag: %d.\n" COLOR_RESET, data->current_train + 1);

    int train_full_reported = 0; // flaga informujaca, czy komunikat o pelnym pociagu zostal wyswietlony

    while (data->generating != -1) // flaga na sprawdzenie zakonczenia dzialania zarzadcy
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0) // wsiadanie do pociagu
        {
            semaphore_wait(sem_train_entry); // zgoda na wejscie

            if (data->passengers_with_bikes > 0 && data->free_bike_spots > 0) // obsluga pasazerow z rowerami
            {
                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Oczekiwanie na otwarcie wejscia dla pasazerow z rowerami.\n" COLOR_RESET);
                semaphore_wait(sem_passengers_bikes);
                printf(COLOR_GREEN "KIEROWNIK POCIAGU: Wejscie dla pasazerow z rowerami zostalo odblokowane.\n" COLOR_RESET);

                int seat = data->free_seat; // id wolnego miejsca w pociagu
                data->train_data[data->current_train][seat] = getpid(); // przypisanie pasazera do miejsca
                data->free_seat++; // zwiekszenie liczby zajetych miejsc
                data->passengers_waiting--; // zmniejszenie liczby oczekujacych pasazerow
                data->passengers_with_bikes--; // zmniejszenie liczby pasazerow z rowerami
                data->free_bike_spots--; // zmniejszenie liczby wolnych miejsc na rowery

                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer z rowerem wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->current_train + 1, seat + 1);

                semaphore_signal(sem_passengers_bikes);
                printf(COLOR_RED "KIEROWNIK POCIAGU: Wejscie dla pasazerow z rowerami zostalo zablokowane.\n" COLOR_RESET);
                usleep(BLOCK_SLEEP*100000);
            }
            else if (data->passengers_waiting > 0) // obsluga pasazerow bez rowerami
            {
                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Oczekiwanie na otwarcie wejscia dla pasazerow bez rowerow.\n" COLOR_RESET);
                semaphore_wait(sem_passengers);
                printf(COLOR_GREEN "KIEROWNIK POCIAGU: Wejscie dla pasazerow bez rowerow zostalo odblokowane.\n" COLOR_RESET);

                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;

                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer bez roweru wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->current_train + 1, seat + 1);

                semaphore_signal(sem_passengers);
                printf(COLOR_RED "KIEROWNIK POCIAGU: Wejscie dla pasazerow bez rowerow zostalo zablokowane.\n" COLOR_RESET);
                usleep(BLOCK_SLEEP*100000);
            }
            semaphore_signal(sem_train_entry); // odblokowanie wejscia dla kolejnych pasazerow
        }

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
            break;
        }
    }
}

#endif // TRAIN_MANAGER_H