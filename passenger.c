#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#define SHM_KEY 9876
#define SEM_KEY 5912
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
    int passengers_with_bikes; // pasazerowie z rowerami
} Data;

volatile sig_atomic_t running = 1; // flaga kontrolna dla procesu
Data *data = NULL; // globalny wskaxnik dla pamieci dzielonej
int memory, semid;

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

void semaphore_wait(int semid)
{
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        perror("PASAZER: Blad blokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int semid)
{
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        perror("PASAZER: Blad odblokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_create()
{
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    if (semid == -1)
    {
        perror("PASAZER: Blad tworzenia semafora");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("PASAZER: Blad inicjalizacji semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_remove()
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("PASAZER: Blad usuwania semafora");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_create()
{
    memory = shmget(SHM_KEY, sizeof(Data), 0600);
    if (memory < 0)
    {
        perror("PASAZER: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_address()
{
    data = (Data *)shmat(memory, NULL, 0);
    if (data == (void *)-1)
    {
        perror("PASAZER: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_detach()
{
    if (shmdt(data) == -1)
    {
        perror("PASAZER: Blad odlaczenia pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    printf("PASAZER: Rozpoczynam proces PID: %d\n", getpid());
    signal(SIGUSR1, handle_signal);
    srand(time(NULL));
    pthread_t keyboard_thread; // deklaracja zmiennej watku
    pthread_create(&keyboard_thread, NULL, keyboard_signal, NULL); // utworzenie nowego watku

    shared_memory_create();
    shared_memory_address();
    //semaphore_create();

    //semaphore_wait(semid);
    //data->passengers_waiting = 0; // inicjalizacja liczby oczekujacych
    //data->generating = 1;        // inicjalizacja flagi generowania
    //semaphore_signal(semid);

    while (running && data->passengers_waiting <= MAX_PASSENGERS * MAX_TRAINS)
    {
        //semaphore_wait(semid);
        int passengers_to_generate = 5 + rand() % 6; // losowa liczba pasazerow 5-10
        data->passengers_waiting ++; // zwiekszenie liczby oczekujacych pasazerow
        int passengers_with_bikes = rand() % (passengers_to_generate + 1); // pasazerowie z rowerami
        data->passengers_with_bikes++; // zwiekszenie liczby pasazerow z rowerami
        printf("PASAZER: Wygenerowano %d nowych pasazerow.\n", passengers_to_generate);
        printf("PASAZER: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);
        //semaphore_signal(semid);
        sleep(3);
    }
    //semaphore_wait(semid);
    data->generating = 0; // flaga generowanie zakonczone
    //semaphore_signal(semid);

    pthread_join(keyboard_thread, NULL); // synchronizacja watku keyboard z glownym
    shared_memory_detach();
    //semaphore_remove();

    printf("PASAZER: Proces zakonczony\n");
    return 0;
}