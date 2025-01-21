#include "functions.h"

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu do zakonczenia dzialania programu

void handle_signal(int signal) // sygnal na przerwanie dzialania
{
    printf(COLOR_MAGENTA "PASAZER: Otrzymalem sygnal %d\n" COLOR_RESET, signal);
    running = 0; // zatrzymanie dzialania
}

void *keyboard_signal(void *arg) // odczyt sygnalu z klawiatury
{
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) // sprawdzenie bledu tcgetattr
    {
        perror(COLOR_RED "PASAZER: Blad tcgetattr" COLOR_RESET);
        return NULL;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // wylaczenie trybu kanonicznego i echa w terminalu

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) // obsluga bledu tcsetattr
    {
        perror(COLOR_RED "PASAZER: Blad tcsetattr" COLOR_RESET);
        return NULL;
    }

    while (running) // petla na odczyt sygnalu
    {
        char ch = getchar();
        if (ch == 14) // ASCII dla CTRL+N
        {
            printf(COLOR_MAGENTA "PASAZER: Przeslano sygnal CTRL+N. Koniec generowania pasazerow.\n" COLOR_RESET);
            running = 0; // zatrzymanie procesu generowania pasazerow
            break;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) // przywrocenie starych ustawien terminala
    {
        perror(COLOR_RED "PASAZER: Blad przywracania tcsetattr" COLOR_RESET);
    }
    return NULL;
}

void create_and_start_keyboard_thread(pthread_t *thread) // tworzenie i uruchomienie watku obslugi klawiatury
{
    if (pthread_create(thread, NULL, keyboard_signal, NULL) != 0)
    {
        handle_error(COLOR_RED "PASAZER: Blad utworzenia watku" COLOR_RESET);
    }
}

void wait_for_keyboard_thread(pthread_t *thread) // oczekiwanie na zakonczenie dzialania watku obslugi klawiatury
{
    if (pthread_join(*thread, NULL) != 0)
    {
        handle_error(COLOR_RED "PASAZER: Blad synchronizacji watku" COLOR_RESET);
    }
}

void passengers_generating(Data *data, int sem_passengers_bikes, int sem_passengers) // generowanie pasazerow i aktualizacja danych wspoldzielonych
{
    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        printf(COLOR_MAGENTA "PASAZER: Oczekiwanie na informacje o pasazerach bez rowerow.\n" COLOR_RESET);
        semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych
        printf(COLOR_GREEN "PASAZER: Otrzymano informacje o pasazerach bez rowerow.\n" COLOR_RESET);

        if (data->free_seat == MAX_PASSENGERS) // sprawdzanie, czy miejsca w pociagu sa dostepne
        {
            semaphore_signal(sem_passengers); // blokada wsiadania gdy nie ma wolnych miejsc
            printf(COLOR_RED "PASAZER: Brak wolnych miejsc w pociagu.\n" COLOR_RESET);
            sleep(BLOCKADE);
            continue;
        }

        int passengers_without_bikes = 5 + rand() % 6; // losowa liczba pasazerow bez rowerow (5-10)
        data->passengers_waiting += passengers_without_bikes; // zwiekszenie liczby oczekujacych pasazerow

        printf(COLOR_MAGENTA "PASAZER: Pojawilo sie %d nowych pasazerow bez rowerow.\n" COLOR_RESET, passengers_without_bikes);
        semaphore_signal(sem_passengers); // odblokowanie danych wspoldzielonych
        printf(COLOR_RED "PASAZER: Przekazanie informacji ile pojawilo sie nowych pasazerow bez rowerow.\n" COLOR_RESET);

        printf(COLOR_MAGENTA "PASAZER: Oczekiwanie na informacje o pasazerach z rowerami.\n" COLOR_RESET);
        semaphore_wait(sem_passengers_bikes);
        printf(COLOR_GREEN "PASAZER: Otrzymano informacje o pasazerach bez rowerow.\n" COLOR_RESET);

        if (data->free_bike_spots > 0)
        {
            int passengers_with_bikes = 4 + rand() % 5; // losowa liczba pasażerów z rowerami (4-8)
            data->passengers_waiting += passengers_with_bikes;
            data->passengers_with_bikes += passengers_with_bikes;
            data->free_bike_spots -= passengers_with_bikes; // aktualizacja wolnych miejsc na rowery

            printf(COLOR_MAGENTA "PASAZER: Pojawilo sie %d nowych pasazerow z rowerow.\n" COLOR_RESET, passengers_with_bikes);
        }
        else
        {
            printf(COLOR_PINK "PASAZER: Brak miejsc dla pasazerow z rowerami.\n" COLOR_RESET);
        }

        semaphore_signal(sem_passengers_bikes);
        printf(COLOR_RED "PASAZER: Zablokowano wejscie dla pasazerow z rowerami.\n" COLOR_RESET);

        sleep(GENERATION_INTERVAL);
    }
    data->generating = 0; // flaga generowanie zakonczone
}

int main()
{
    srand(time(NULL)); // generator liczb

    int memory; // id pamieci dzielonej
    Data *data = NULL; // wskaznik na strukture danych wspoldzielonych

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    int sem_passengers_bikes = semaphore_create(SEM_KEY_PASSENGERS_BIKES);
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    if (semctl(sem_passengers_bikes, 0, SETVAL, 1) == -1)
    {
        handle_error("PASAZER: Blad inicjalizacji semafora dla pasazerow z rowerami");
    }
    if (semctl(sem_passengers, 0, SETVAL, 1) == -1)
    {
        handle_error("PASAZER: Blad inicjalizacji semafora dla pasazerow bez rowerow");
    }

    semaphore_signal(sem_passengers_bikes);
    semaphore_signal(sem_passengers);

    pthread_t keyboard_thread; // deklaracja zmiennej watku
    create_and_start_keyboard_thread(&keyboard_thread); // semafor do zarzadzania pasazerami

    data->generating = 1; // flaga - generowanie rozpoczete

    passengers_generating(data, sem_passengers_bikes, sem_passengers);

    wait_for_keyboard_thread(&keyboard_thread);

    printf(COLOR_PINK "PASAZER: Proces zakonczony\n" COLOR_RESET);
    return 0;
}