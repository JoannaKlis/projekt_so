#include "functions.h"
#include "train_manager.h"
#include "signal.h"

int main()
{
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    if (semctl(sem_train_entry, 0, SETVAL, 1) == -1) 
    {
        handle_error("KIEROWNIK POCIAGU: Blad inicjalizacji semafora dla wejścia do pociągu");
    }
    if (semctl(sem_passengers_bikes, 0, SETVAL, 1) == -1)
    {
        handle_error("KIEROWNIK POCIAGU: Blad inicjalizacji semafora dla pasazerow z rowerami");
    }
    if (semctl(sem_passengers, 0, SETVAL, 1) == -1)
    {
        handle_error("KIEROWNIK POCIAGU: Blad inicjalizacji semafora dla pasazerow bez rowerow");
    }

    semaphore_signal(sem_train_entry);
    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    handle_passenger(data, sem_passengers_bikes, sem_passengers, sem_train_entry);

    shared_memory_detach(data);
    semaphore_remove(sem_train_entry);
    semaphore_remove(sem_passengers_bikes);
    semaphore_remove(sem_passengers);

    return 0;
}