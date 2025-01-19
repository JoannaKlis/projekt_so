#include "functions.h"

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu

void handle_signal(int signal) // sygnal na przerwanie dzialania
{
    printf("PASAZER: Otrzymalem sygnal %d\n", signal);
    running = 0;
}

void *keyboard_signal(void *arg)
{
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) // sprawdzenie bledu tcgetattr
    {
        perror("PASAZER: Blad tcgetattr");
        return NULL;
    }
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) // sprawdzenie bledu tcsetattr
    {
        perror("PASAZER: Blad tcsetattr");
        return NULL;
    }

    while (running) // petla na odczyt sygnalu
    {
        char ch = getchar();
        if (ch == 14) // ASCII dla CTRL+N
        {
            printf("PASAZER: Przeslano sygnal CTRL+N. Koniec generowania pasazerow.\n");
            running = 0;
            break;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) // przywrocenie starych ustawien terminala
    {
        perror("PASAZER: Blad przywracania tcsetattr");
    }
    return NULL;
}

void create_and_start_keyboard_thread(pthread_t *thread)
{
    if (pthread_create(thread, NULL, keyboard_signal, NULL) != 0)
    {
        handle_error("PASAZER: Blad utworzenia watku");
    }
}

void wait_for_keyboard_thread(pthread_t *thread)
{
    if (pthread_join(*thread, NULL) != 0)
    {
        handle_error("PASAZER: Blad synchronizacji watku");
    }
}

void passengers_generating(Data *data, int sem_passengers)
{
    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych

        if (data->free_seat == MAX_PASSENGERS) // blokada wsiadania jesli pociag odjedzie
        {
            semaphore_signal(sem_passengers);
            sleep(1);
            continue;
        }

        int passengers_to_generate = 5 + rand() % 6; // losowa liczba pasazerow 5-10
        data->passengers_waiting += passengers_to_generate; // zwiekszenie liczby oczekujacych pasazerów
        data->passengers_with_bikes += rand() % (passengers_to_generate + 1); // dodanie pasazerów z rowerami

        printf("PASAZER: Wygenerowano %d nowych pasazerow.\n", passengers_to_generate);
        semaphore_signal(sem_passengers); // odblokowanie danych wspoldzielonych

        sleep(3);
    }
    data->generating = 0; // flaga generowanie zakonczone
}

int main()
{
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    pthread_t keyboard_thread; // deklaracja zmiennej watku
    create_and_start_keyboard_thread(&keyboard_thread);
    
    data->generating = 1; // flaga generowanie rozpoczete

    passengers_generating(data, sem_passengers);

    wait_for_keyboard_thread(&keyboard_thread);

    printf("PASAZER: Proces zakonczony\n");
    return 0;
}