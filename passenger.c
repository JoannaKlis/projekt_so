#include "functions.h"
#include "passenger.h"
#include "signal.h"

int passengers_out = 0; // flaga czyszczenia miejsca w pociagu

int main()
{
    srand(time(NULL)); // do losowosci pidow
    setbuf(stdout, NULL);

    int memory; // id pamieci dzielonej
    Data *data = NULL; // wskaznik na strukture danych wspoldzielonych

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    semaphore_signal(sem_train_entry);
    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    data->generating = 1; // flaga - generowanie rozpoczete

    passengers_generating(data, sem_passengers_bikes, sem_passengers, sem_train_entry);
    
    return 0;
}