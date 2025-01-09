#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#define SHM_KEY 1234
#define MAX_PASSENGERS 10
#define MAX_TRAINS 4

typedef struct
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[4][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery
} Data;

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

void notify_disembarkation(int train, int station) 
{
    printf("PASAZER: Pociag %d wysadzil pasazerow na stacji %d.\n", train, station);
}

int main()
{
    printf("PASAZER: rozpoczynam proces PID: %d\n", getpid());
    signal(SIGUSR1, handle_signal);
    srand(time(NULL));
    pthread_t keyboard_thread; // deklaracja zmiennej watku
    pthread_create(&keyboard_thread, NULL, keyboard_signal, NULL); // utworzenie nowego watku

    int shmid = shmget(SHM_KEY, sizeof(Data), 0600); // identyfikator pamieci dzielonej
    if (shmid < 0) 
    {
        perror("PASAZER: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    Data *data = (Data *)shmat(shmid, NULL, 0); // dolaczenie segmentu pamieci dzielonej
    if (data == (void *)-1) 
    {
        perror("PASAZER: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }

    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        int passengers_to_generate = 10 + rand() % 11; // losowa liczba pasazerow od 10 do 20
        data->passengers_waiting += passengers_to_generate; // zwiekszenie liczby oczekujacych pasazerow
        //int passengers_with_bikes = rand() % (passengers_to_generate + 1); // pasazerowie z rowerami
        //data->free_bike_spots = passengers_with_bikes; // zwiekszenie liczby pasazerow z rowerami
        printf("PASAZER: Wygenerowano %d nowych pasazerow.\n", passengers_to_generate);
        printf("PASAZER: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);

        if (data->current_train == 2) //stacja 2
        {
            notify_disembarkation(data->current_train, 2);
        }

        sleep(2);
    }

    data->generating = 0; // flaga generowanie zakonczone
    pthread_join(keyboard_thread, NULL); // synchronizacja watku keyboard z glownym
    shmdt(data); // odlaczenie struktur

    if (shmdt(data) == -1) 
    {
    perror("PASAZER: Blad odlaczenia pamieci dzielonej");
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
    perror("PASAZER: Blad usuniecia segmentu pamieci dzielonej");
    }

    printf("PASAZER: Proces zakonczony\n");
    return 0;
}