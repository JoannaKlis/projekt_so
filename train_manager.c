#include "functions.h"
#include "train_manager.h"
#include "signal.h"

pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex dla pamięci współdzielonej

int main()
{
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    if (data == NULL) {
        handle_error("Failed to initialize shared memory data");
    }

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    semaphore_signal(sem_train_entry);
    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    handle_passenger(data, sem_passengers_bikes, sem_passengers, sem_train_entry);

    shared_memory_detach(data);
    semaphore_remove(sem_passengers_bikes);
    semaphore_remove(sem_passengers);
    semaphore_remove(sem_train_entry);

    return 0;
}