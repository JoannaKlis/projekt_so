#ifndef TRAIN_MANAGER_H
#define TRAIN_MANAGER_H

void handle_passenger(Data *data, int sem_passengers_bikes, int sem_passengers, int sem_train_entry) 
{
    printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d pojawil sie na stacji 1.\n" COLOR_RESET, data->current_train + 1);

    int train_full_reported = 0;

    while (data->generating != -1) 
    {
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0) 
        {
            semaphore_wait(sem_train_entry);

            if (data->passengers_with_bikes > 0 && data->free_bike_spots > 0) 
            {
                semaphore_wait(sem_passengers_bikes);
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;
                data->passengers_with_bikes--;
                data->free_bike_spots--;

                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer %d z rowerem wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->passenger_pid, data->current_train + 1, seat + 1);

                semaphore_signal(sem_passengers_bikes);
            }
            else if (data->passengers_waiting > 0) 
            {
                semaphore_wait(sem_passengers);
                int seat = data->free_seat;
                data->train_data[data->current_train][seat] = getpid();
                data->free_seat++;
                data->passengers_waiting--;

                printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pasazer %d bez roweru wsiadl do pociagu %d i zajal miejsce %d.\n" COLOR_RESET, data->passenger_pid, data->current_train + 1, seat + 1);

                semaphore_signal(sem_passengers);
            }
            semaphore_signal(sem_train_entry);
        }

        if (data->free_seat == MAX_PASSENGERS && !train_full_reported) 
        {
            printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d jest gotowy do odjazdu.\n" COLOR_RESET, data->current_train + 1);
            train_full_reported = 1;
        }

        if (data->free_seat < MAX_PASSENGERS) 
        {
            train_full_reported = 0;
        }

        if (data->generating == -1) 
        {
            for (int i = 0; i < MAX_PASSENGERS_GENERATE; i++) 
            {
                if (data->passenger_pids[i] != 0) 
                {
                    if (waitpid(data->passenger_pids[i], NULL, 0) == -1) 
                    {
                        perror("KIEROWNIK POCIAGU: Blad podczas oczekiwania na pasazera");
                    } else {
                        printf(COLOR_GREEN "KIEROWNIK POCIAGU: Pasazer %d zakonczyl swoj kurs.\n" COLOR_RESET, data->passenger_pids[i]);
                    }
                    data->passenger_pids[i] = 0;
                }
            }

            printf(COLOR_ORANGE "KIEROWNIK POCIAGU (PID %d): Brak oczekujacych pasazerow\nKONIEC DZIALANIA.\n" COLOR_RESET, getpid());
            break;
        }

        sleep(BLOCK_SLEEP * TRAIN_ARRIVAL_TIME);

        data->free_seat = 0;
        data->free_bike_spots = MAX_BIKES;
        data->current_train = (data->current_train + 1) % MAX_TRAINS;
    }
}

#endif // TRAIN_MANAGER_H