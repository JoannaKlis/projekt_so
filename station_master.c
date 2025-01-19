#include "functions.h"

void station_master(Data *data, int sem_passengers)
{
    while (1)  // dzialanie zarzadcy
    {
        semaphore_wait(sem_passengers); // blokada podczas zarzadzania pasazerami

        if (data->passengers_waiting == 0 && !data->generating) 
        {
            printf("ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n");
            data->generating = -1; // flaga - zarzadca konczy dzialanie
            semaphore_signal(sem_passengers);
            break;
        }

       if (data->passengers_waiting > 0)
        {
            printf("ZARZADCA: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);
            printf("ZARZADCA: Pociag %d przyjechal na stacje 1.\n", data->current_train);

            sleep(TRAIN_DEPARTURE_TIME); // czas do odjazdu pociągu

            printf("ZARZADCA: Pociag %d odjezdza ze stacji 1.\n", data->current_train);

            data->free_seat = MAX_PASSENGERS; // zablokowanie dalszego wsiadania
            sleep(TRAIN_ARRIVAL_TIME); // podroz na stacje 2

            printf("ZARZADCA: Pociag %d dotarl na stacje 2.\n", data->current_train);
            printf("ZARZADCA: Pasazerowie wysiadaja na stacji 2.\n");

            sleep(1); // czas wysiadania

            data->free_seat = 0; // reset miejsc
            data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery

            printf("ZARZADCA: Pociag %d wrocil na stacje 1.\n", data->current_train);

            data->current_train = (data->current_train + 1) % MAX_TRAINS; // nastepny pociag
        }
        semaphore_signal(sem_passengers);
    }
}

int main() 
{
    int memory;
    Data *data = NULL;

    srand(time(NULL));

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

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

    station_master(data, sem_passengers);

    waitpid(train_manager_pid, NULL, 0); // czekanie na zakonczenie procesow potomnych
    waitpid(passenger_pid, NULL, 0);

    shared_memory_detach(data);
    shared_memory_remove(memory);

    return 0;
}