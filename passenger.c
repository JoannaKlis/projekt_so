#include "functions.h"

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu

void handle_signal(int signal) // sygnal na przerwanie dzialania
{
    printf("PASAZER: Otrzymalem sygnal %d\n", signal);
    running = 0;
}

void *keyboard_signal(void *arg)
{
    struct termios oldt, newt; // deklaracja zmiennych terminalowych
    tcgetattr(STDIN_FILENO, &oldt); // pobranie obecnych ustawien terminala
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // modyfikacja ustawien terminala

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

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // stare ustawienia
    return NULL; // koniec watku
}

int main()
{
    int memory;
    Data *data = NULL;

    printf("PASAZER: Rozpoczynam proces PID: %d\n", getpid());
    signal(SIGUSR1, handle_signal);
    srand(time(NULL));
    pthread_t keyboard_thread; // deklaracja zmiennej watku
    pthread_create(&keyboard_thread, NULL, keyboard_signal, NULL); // utworzenie nowego watku

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    data->generating = 1; // flaga generowanie rozpoczete

    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        int passengers_to_generate = 5 + rand() % 6; // losowa liczba pasazerow 5-10
        data->passengers_waiting ++; // zwiekszenie liczby oczekujacych pasazerow
        int passengers_with_bikes = rand() % (passengers_to_generate + 1); // pasazerowie z rowerami
        data->passengers_with_bikes++; // zwiekszenie liczby pasazerow z rowerami
        printf("PASAZER: Wygenerowano %d nowych pasazerow.\n", passengers_to_generate);
        //printf("PASAZER: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);
        sleep(3);
    }
    data->generating = 0; // flaga generowanie zakonczone

    pthread_join(keyboard_thread, NULL); // synchronizacja watku keyboard z glownym
    shared_memory_detach(data);

    printf("PASAZER: Proces zakonczony\n");
    return 0;
}