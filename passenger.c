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
        handle_error(COLOR_RED"PASAZER: Blad utworzenia watku" COLOR_RESET);
    }
}

void wait_for_keyboard_thread(pthread_t *thread) // oczekiwanie na zakonczenie dzialania watku obslugi klawiatury
{
    if (pthread_join(*thread, NULL) != 0)
    {
        handle_error(COLOR_RED "PASAZER: Blad synchronizacji watku" COLOR_RESET);
    }
}

void passengers_generating(Data *data, int sem_passengers) // generowanie pasazerow i aktualizacja danych wspoldzielonych
{
    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        semaphore_wait(sem_passengers); // zablokowanie dostepu do danych wspoldzielonych

        if (data->free_seat == MAX_PASSENGERS) // sprawdzanie, czy miejsca w pociagu sa dostepne
        {
            semaphore_signal(sem_passengers); // blokada wsiadania gdy nie ma wolnych miejsc
            //sleep(1);
            continue;
        }

        int passengers_to_generate = 5 + rand() % 6; // losowa liczba pasazerow 5-10
        data->passengers_waiting += passengers_to_generate; // zwiekszenie liczby oczekujacych pasazerów
        data->passengers_with_bikes += rand() % (passengers_to_generate + 1); // dodanie pasazerów z rowerami

        printf(COLOR_MAGENTA "PASAZER: Wygenerowano %d nowych pasazerow.\n" COLOR_RESET, passengers_to_generate);
        semaphore_signal(sem_passengers); // odblokowanie danych wspoldzielonych

        //sleep(3);
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
    int sem_passengers = semaphore_create(SEM_KEY_PASSENGERS);

    pthread_t keyboard_thread; // deklaracja zmiennej watku
    create_and_start_keyboard_thread(&keyboard_thread); // semafor do zarzadzania pasazerami

    data->generating = 1; // flaga - generowanie rozpoczete

    passengers_generating(data, sem_passengers);

    wait_for_keyboard_thread(&keyboard_thread);

    printf(COLOR_ORANGE "PASAZER: Proces zakonczony\n" COLOR_RESET);
    return 0;
}