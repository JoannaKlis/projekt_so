#ifndef SIGNAL_H
#define SIGNAL_H
#include "functions.h"
#include <sys/select.h>

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu do zakonczenia dzialania programu
volatile sig_atomic_t signal1 = 1; // flaga dla sygnalu CTRL+K
volatile sig_atomic_t signal2 = 1; // flaga dla sygnalu CTRL+L

pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER; // mutez uzywany do synchronizacjji dostepu do flag

void set_running(int value)  // funkcja ustawia wartosc flagi running w sposob bezpieczny dla watku
{
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w set_running" COLOR_RESET);
        return; // gdy nastapi blad blokowania, koniec dzialania
    }
    running = value; // nowa wartosc flagi
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w set_running" COLOR_RESET);
    }
}

int get_running() // zwara wartosc running
{
    int value;
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w get_running" COLOR_RESET);
        return -1; // gdy nastapi blad blokowania
    }
    value = running; // pobranie wartosci flagi
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w get_running" COLOR_RESET);
    }
    return value;
}

void set_signal1(int value) // ustawienie wartosci flagi dla sygnalu 1
{
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w set_signal1" COLOR_RESET);
        return;
    }
    signal1 = value;
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w set_signal1" COLOR_RESET);
    }
}

int get_signal1() // zwrocenie wartosci flagi dla sygnalu 1
{
    int value;
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w get_signal1" COLOR_RESET);
        return -1;
    }
    value = signal1;
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w get_signal1" COLOR_RESET);
    }
    return value;
}

void set_signal2(int value) // ustawia wartosc flagi dla sygnalu 2
{
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w set_signal2" COLOR_RESET);
        return;
    }
    signal2 = value;
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w set_signal2" COLOR_RESET);
    }
}

int get_signal2() // zwraca wartosc flagi dla sygnalu2
{
    int value;
    if (pthread_mutex_lock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy blokowaniu mutexu w get_signal2" COLOR_RESET);
        return -1;
    }
    value = signal2;
    if (pthread_mutex_unlock(&signal_mutex) != 0) 
    {
        perror(COLOR_RED "SIGNAL: Blad przy odblokowywaniu mutexu w get_signal2" COLOR_RESET);
    }
    return value;
}

void *keyboard_signal(void *arg) {
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("SIGNAL: Error in tcgetattr");
        return NULL;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0; 

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        perror("SIGNAL: Error in tcsetattr");
        return NULL;
    }

    fd_set readfds;
    struct timeval timeout;

    while (running) { 
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000; 

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

        if (ret > 0) {
            char ch = getchar();
            if (ch == 14) {
                set_running(0);
                break;
            } else if (ch == 11) {
                set_signal1(0); 
            } else if (ch == 12) {
                set_signal2(0); 
            }
        }
        usleep(BLOCK_SLEEP * 50000);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) {
        perror("SIGNAL: Error restoring terminal settings");
    }
    return NULL;
}

void handle_continue(int signal)  // przywrocenie czekania na kolejny sygnal
{
    struct termios newt;
    if (tcgetattr(STDIN_FILENO, &newt) == -1) 
    {
        perror(COLOR_RED "SIGNAL: Blad tcgetattr w handle_continue" COLOR_RESET);
        return;
    }
    newt.c_lflag |= (ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) 
    {
        perror(COLOR_RED "SIGNAL: Blad tcsetattr w handle_continue" COLOR_RESET);
    }
    printf(COLOR_RED "SIGNAL: Proces wznowiony.\n" COLOR_RESET);
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