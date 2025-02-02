#include "functions.h"
#include "passenger.h"
#include "signal.h"

int main()
{
    setbuf(stdout, NULL);
    srand(time(NULL)); // generator liczb

    int memory; // id pamieci dzielonej
    Data *data = NULL; // wskaznik na strukture danych wspoldzielonych

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu
    int sem_passengers_entry = semaphore_create(SEM_KEY_PASSENGERS_ENTRY);
    int sem_passengers_exit = semaphore_create(SEM_KEY_PASSENGERS_EXIT);
    int sem_bike_entry = semaphore_create(SEM_KEY_BIKE_ENTRY);
    int sem_bike_exit = semaphore_create(SEM_KEY_BIKE_EXIT);

    //pthread_t keyboard_thread; // deklaracja zmiennej watku
    //create_and_start_keyboard_thread(&keyboard_thread); // semafor do zarzadzania pasazerami

    passenger_process(data, sem_passengers_entry, sem_bike_entry);

    //wait_for_keyboard_thread(&keyboard_thread);

    printf(COLOR_PINK "PASAZER: Proces zakonczony\n" COLOR_RESET);
    return 0;
}