#ifndef SIGNAL_H
#define SIGNAL_H
#include "functions.h"

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu do zakonczenia dzialania programu
volatile sig_atomic_t signal1 = 1; // flaga dla sygnalu CTRL+K
volatile sig_atomic_t signal2 = 1; // flaga dla sygnalu CTRL+L

void handle_signal(int signal) // sygnal na przerwanie dzialania
{
    printf(COLOR_PINK "SIGNAL: Otrzymalem sygnal %d\n" COLOR_RESET, signal);
    running = 0; // zatrzymanie dzialania
}

void *keyboard_signal(void *arg) // odczyt sygnalu z klawiatury
{
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) // sprawdzenie bledu tcgetattr
    {
        perror(COLOR_RED "SIGNAL: Blad tcgetattr" COLOR_RESET);
        return NULL;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // wylaczenie trybu kanonicznego i echa w terminalu

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) // obsluga bledu tcsetattr
    {
        perror(COLOR_RED "SIGNAL: Blad tcsetattr" COLOR_RESET);
        return NULL;
    }

    while (running) // petla na odczyt sygnalu
    {
        char ch = getchar();
        if (ch == 14) // ASCII dla CTRL+N
        {
            printf(COLOR_PINK "SIGNAL: Przeslano sygnal CTRL+N. Koniec generowania pasazerow.\n" COLOR_RESET);
            running = 0; // zatrzymanie procesu generowania pasazerow
            break;
        }
        else if (ch == 11)
        {
            printf(COLOR_PINK "SIGNAL: Przeslano sygnal CTRL+K. Pociag odjezdza\n" COLOR_RESET);
            signal1 = 0;
            break;
        }
        else if (ch == 12)
        {
            printf(COLOR_PINK "SIGNAL: Przeslano sygnal CTRL+L. Pasazerowie nie moga wejsc\n" COLOR_RESET);
            signal2 = 0;
            break;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) // przywrocenie starych ustawien terminala
    {
        perror(COLOR_RED "SIGNAL: Blad przywracania tcsetattr" COLOR_RESET);
    }
    return NULL;
}

void create_and_start_keyboard_thread(pthread_t *thread) // tworzenie i uruchomienie watku obslugi klawiatury
{
    if (pthread_create(thread, NULL, keyboard_signal, NULL) != 0)
    {
        handle_error(COLOR_RED "SIGNAL: Blad utworzenia watku" COLOR_RESET);
    }
}

void wait_for_keyboard_thread(pthread_t *thread) // oczekiwanie na zakonczenie dzialania watku obslugi klawiatury
{
    if (pthread_join(*thread, NULL) != 0)
    {
        handle_error(COLOR_RED "SIGNAL: Blad synchronizacji watku" COLOR_RESET);
    }
}

#endif // SIGNAL_H