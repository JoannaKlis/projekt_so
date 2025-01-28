#include "functions.h"
#include "passenger.h"
#include "signal.h"

int main()
{
    srand(time(NULL)); // generator liczb

    int memory; // id pamieci dzielonej
    Data *data = NULL; // wskaznik na strukture danych wspoldzielonych

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    if (semctl(sem_train_entry, 0, SETVAL, 1) == -1) 
    {
        handle_error("PASAZER: Blad inicjalizacji semafora dla wejścia do pociągu");
    }
    if (semctl(sem_passengers_bikes, 0, SETVAL, 1) == -1)
    {
        handle_error("PASAZER: Blad inicjalizacji semafora dla pasazerow z rowerami");
    }
    if (semctl(sem_passengers, 0, SETVAL, 1) == -1)
    {
        handle_error("PASAZER: Blad inicjalizacji semafora dla pasazerow bez rowerow");
    }

    semaphore_signal(sem_train_entry);
    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    pthread_t keyboard_thread; // deklaracja zmiennej watku
    create_and_start_keyboard_thread(&keyboard_thread); // semafor do zarzadzania pasazerami

    passengers_generating(data, sem_passengers_bikes, sem_passengers, sem_train_entry);

    wait_for_keyboard_thread(&keyboard_thread);

    printf(COLOR_PINK "PASAZER: Proces zakonczony\n" COLOR_RESET);
    return 0;
}