#include "functions.h"
#include "train_manager.h"
#include "signal.h"

pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        handle_error("KIEROWNIK POCIAGU: brak takiego pociagu o tym numerze");
    }

    int train_id = atoi(argv[1]);  // pobranie numeru pociagu
    printf("KIEROWNIK POCIAGU: Pociag %d jest gotowy.\n", train_id);

    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    semaphore_signal(sem_train_entry);
    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    data->current_train = train_id;  // ustawienie numeru aktualnego pociagu

    handle_passenger(data, sem_passengers_bikes, sem_passengers, sem_train_entry);

    shared_memory_detach(data);
    
    return 0;
}