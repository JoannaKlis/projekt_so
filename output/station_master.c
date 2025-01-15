#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_KEY 9876
#define SEM_KEY 5912
#define MAX_TRAINS 4
#define MAX_PASSENGERS 10
#define MAX_BIKES 5

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery 
    int passengers_with_bikes; // pasazerowie z rowerami
} Data;

int memory, semid;
Data *data = NULL; // globalny wskaznik dla pamieci dzielonej

void semaphore_wait(int semid)
{
    struct sembuf lock = {0, -1, 0};
    if (semop(semid, &lock, 1) == -1)
    {
        perror("ZARZADCA: Blad blokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int semid)
{
    struct sembuf unlock = {0, 1, 0};
    if (semop(semid, &unlock, 1) == -1)
    {
        perror("ZARZADCA: Blad odblokowania semafora");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_create()
{
    memory = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (memory < 0)
    {
        perror("ZARZADCA: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_address()
{
    data = (Data *)shmat(memory, NULL, 0);
    if (data == (void *)-1)
    {
        perror("ZARZADCA: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
}

void semaphore_create()
{
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0600);
    if (semid == -1)
    {
        perror("ZARZADCA: Blad tworzenia semafora");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("ZARZADCA: Blad inicjalizacji semafora");
        exit(EXIT_FAILURE);
    }
}

void semaphore_remove()
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("ZARZADCA: Blad usuwania semafora");
        exit(EXIT_FAILURE);
    }
}

void shared_memory_detach()
{
    if (shmdt(data) == -1)
    {
        perror("ZARZADCA: Blad odlaczenia pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
    if (shmctl(memory, IPC_RMID, 0) == -1)
    {
        perror("ZARZADCA: Blad usuwania segmentu pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
}

void station_master(Data *data) 
{
    while (1)  // dzialanie zarzadcy
    {
        //semaphore_wait(semid);
        if (data->passengers_waiting == 0 && !data->generating) 
        {
            printf("ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n");
            data->generating = -1; // flaga - zarzadca konczy dzialanie
            //semaphore_signal(semid);
            break;
        }
        if (data->passengers_waiting > 0)
        {
            printf("ZARZADCA: Pociag %d przyjechal na stacje 1.\n", data->current_train);
            //semaphore_signal(semid);
            sleep(5); // czas na wsiadanie pasazerow

            //semaphore_wait(semid);
            if (data->free_seat == MAX_PASSENGERS) // tylko pelny pociag moze opuscic stacje
            {
                printf("ZARZADCA: Pociag %d odjezdza ze stacji 1.\n", data->current_train);
                //semaphore_signal(semid);
                sleep(4); // czas na przyjazd do stacji 2

                //semaphore_wait(semid);
                printf("ZARZADCA: Pociag %d dotarl na stacje 2.\n", data->current_train);
                sleep(1);

                printf("ZARZADCA: Pasazerowie wysiadaja na stacji 2.\n");
                sleep(2);

                data->free_seat = 0; // reset wolnych miejsc po wysadzeniu
                data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery
            }

            data->current_train = (data->current_train + 1) % MAX_TRAINS; // cykl z pociagami

            printf("ZARZADCA: Pociag %d wrocil na stacje 1.\n", data->current_train);
            //semaphore_signal(semid);
        }
    }
}

int main() 
{
    srand(time(NULL));

    shared_memory_create();
    shared_memory_address();
    //semaphore_create();

    //semaphore_wait(semid);
    //data->passengers_waiting = 0; // inicjalizacja liczby oczekujacych
    //data->generating = 1;        // inicjalizacja flagi generowania
    //semaphore_signal(semid);

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery
    data->passengers_waiting = 0; //ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

    pid_t train_manager_pid = fork();
    if (train_manager_pid < 0)
    {
        perror("ZARZADCA: Blad fork dla train_manager");
        exit(EXIT_FAILURE);
    }
    if (train_manager_pid == 0)
    {
        execl("./train_manager", "./train_manager", NULL);
        perror("ZARZADCA: Blad execl pliku train_manager");
        exit(EXIT_FAILURE);
    }

    pid_t passenger_pid = fork();
    if (passenger_pid < 0)
    {
        perror("ZARZADCA: Blad fork dla passenger");
        exit(EXIT_FAILURE);
    }
    if (passenger_pid == 0) 
    {
        execl("./passenger", "./passenger", NULL);
        perror("ZARZADCA: Blad execl pliku passenger");
        exit(EXIT_FAILURE);
    }

    station_master(data);

    //semaphore_wait(semid);
    //data->generating = 0; // flaga generowanie zakonczone
    //semaphore_signal(semid);

    waitpid(train_manager_pid, NULL, 0); // czekanie na zakonczenie procesow potomnych
    waitpid(passenger_pid, NULL, 0); 

    shared_memory_detach();
    //semaphore_remove();
    return 0;
}