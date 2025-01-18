#include "functions.h"

void station_master(Data *data) 
{
    while (1)  // dzialanie zarzadcy
    {
        if (data->passengers_waiting == 0 && !data->generating) 
        {
            printf("ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n");
            data->generating = -1; // flaga - zarzadca konczy dzialanie
            break;
        }

        if (data->passengers_waiting > 0)
        {
            printf("ZARZADCA: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);
            data->passengers_waiting--;

            printf("ZARZADCA: Pociag %d przyjechal na stacje 1.\n", data->current_train);

            if (run_for_Ttime()) // odjazd pociagu po T czasie
            {
                printf("ZARZADCA: Pociag %d odjezdza ze stacji 1.\n", data->current_train);
                sleep(2);

                printf("ZARZADCA: Pociag %d dotarl na stacje 2.\n", data->current_train);
                sleep(1);

                printf("ZARZADCA: Pasazerowie wysiadaja na stacji 2.\n");
                sleep(TRAIN_ARRIVAL_TIME); // powrót na stacje glowna po Ti czasie

                data->free_seat = 0; // reset wolnych miejsc po wysadzeniu
                data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery
            }

            printf("ZARZADCA: Pociag %d wrocil na stacje 1.\n", data->current_train);

            data->current_train = (data->current_train + 1) % MAX_TRAINS; // cykl z pociagami
        }
    }
}

int main() 
{
    int memory;
    Data *data = NULL;

    srand(time(NULL));

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery
    data->passengers_waiting = 0; //ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

    pid_t train_manager_pid = fork();
    if (train_manager_pid < 0)
    {
        perror("ZARZADCA: Blad fork dla train_manager");
        exit(EXIT_FAILURE);
    }
    if (train_manager_pid == 0)
    {
        execl("./train_manager", "./train_manager", NULL);
        perror("ZARZADCA: Blad execl pliku train_manager");
        exit(EXIT_FAILURE);
    }

    pid_t passenger_pid = fork();
    if (passenger_pid < 0)
    {
        perror("ZARZADCA: Blad fork dla passenger");
        exit(EXIT_FAILURE);
    }
    if (passenger_pid == 0) 
    {
        execl("./passenger", "./passenger", NULL);
        perror("ZARZADCA: Blad execl pliku passenger");
        exit(EXIT_FAILURE);
    }

    station_master(data);

    waitpid(train_manager_pid, NULL, 0); // czekanie na zakonczenie procesow potomnych
    waitpid(passenger_pid, NULL, 0); 

    shared_memory_detach(data);
    shared_memory_remove(memory);

    return 0;
}