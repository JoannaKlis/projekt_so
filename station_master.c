#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_KEY 1234
#define MAX_TRAINS 4
#define MAX_PASSENGERS 10
#define SEM_KEY_PASSENGERS 5678
#define SEM_KEY_BIKES 5679
#define MAX_BIKES 5

typedef struct 
{
    int current_train; // aktualny pociag na stacji
    int free_seat;     // numer 1 wolnego miejsca w pociagu
    int train_data[MAX_TRAINS][MAX_PASSENGERS]; // tablica PIDow pasazerow w pociagach
    int passengers_waiting; // liczba oczekujacych pasazerow
    int generating; // flaga czy pasazerowie sa generowani
    int free_bike_spots; // liczba wolnych miejsc na rowery 
} Data;

void init_shared_memory(int *shmid, Data **data) // tworzenie pamieci dzielonej
{
    *shmid = shmget(SHM_KEY, sizeof(Data), IPC_CREAT | 0600);
    if (*shmid < 0) 
    {
        perror("ZARZADCA: Blad utworzenia segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
    *data = (Data *)shmat(*shmid, NULL, 0);
    if (*data == (void *)-1)
    {
        perror("ZARZADCA: Blad dostepu do segmentu pamieci dzielonej!");
        exit(EXIT_FAILURE);
    }
} 

void cleanup_shared_memory(int shmid, Data *data) // czyszczenie pamieci
{
    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
}

void init_semaphores() //inicjacja semaforow
{
    int sem_passengers = semget(SEM_KEY_PASSENGERS, 1, IPC_CREAT | 0600);
    int sem_bikes = semget(SEM_KEY_BIKES, 1, IPC_CREAT | 0600);
    semctl(sem_passengers, 0, SETVAL, 1);
    semctl(sem_bikes, 0, SETVAL, 1);
}

void station_master(Data *data, int N) 
{
    while (1)  // dzialanie zarzadcy
    {
        if (data->passengers_waiting == 0 && !data->generating) 
        {
            printf("ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n");
            break;
        }
        
        printf("ZARZADCA: Pociag %d przyjechal na stacje 1.\n", data->current_train);
        printf("ZARZADCA: Liczba wszystkich oczekujacych: %d.\n", data->passengers_waiting);
        sleep(2); // czas na wsiadanie pasazerow

        printf("ZARZADCA: Pasazerowie wsiadaja do pociagu %d.\n", data->current_train);
        while (data->free_seat < MAX_PASSENGERS && data->passengers_waiting > 0)
        {
            int passenger_pid = rand() % 10000; // pid pasazerow
            int has_bike = rand() % 2; // czy pasazer ma rower

            data->train_data[data->current_train][data->free_seat] = passenger_pid; // dodanie pasazera do pociagu
            if (has_bike) 
            {
                printf("ZARZADCA: Pasazer %d z rowerem wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            } else 
            {
                printf("ZARZADCA: Pasazer %d wszedl do pociagu %d i zajal miejsce %d.\n",passenger_pid, data->current_train, data->free_seat);
            }

            data->free_seat++;
            data->passengers_waiting--;
        }
        sleep(2);

        printf("ZARZADCA: Pociag %d odjezdza ze stacji 1.\n", data->current_train);
        sleep(4); // czas na przyjazd do stacji 2

        printf("ZARZADCA: Pociag %d dotarl na stacje 2.\n", data->current_train);
        printf("ZARZADCA: Pasazerowie wysiadaja na stacji 2.\n");
        data->free_seat = 0; // reset wolnych miejsc po wysadzeniu

        printf("ZARZADCA: Pociag %d odjezdza ze stacji 2.\n", data->current_train);
        sleep(4); // czas na powrot do stacji 1

        data->current_train = (data->current_train + 1) % N; // cykl z pociagami
        data->free_seat = 0; // reset wolnych miejsc
        data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery

        printf("ZARZADCA: Przygotowano pociag %d na powrot do stacji 1.\n", data->current_train);
    }
}

int main() 
{
    srand(time(NULL));
    int shmid;
    Data *data;

    init_shared_memory(&shmid, &data); // inicjalizacja pamieci dzielonej
    init_semaphores(); // inicjalizacja semaforow

    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; //reset miejsc na rowery
    data->passengers_waiting = 0; //ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani
    for (int i = 0; i < MAX_TRAINS; i++) 
    {
        for (int j = 0; j < MAX_PASSENGERS; j++) 
        {
            data->train_data[i][j] = 0; // 0 to wolne miejsce
        }
    }

    pid_t train_manager_pid = fork();
    if (train_manager_pid == 0)
    {
        execl("./train_manager", "./train_manager", NULL);
        perror("ZARZADCA: Blad execl pliku train_manager");
        exit(EXIT_FAILURE);
    }

    pid_t passenger_pid = fork();
    if (passenger_pid == 0) 
    {
        execl("./passenger", "./passenger", NULL);
        perror("ZARZADCA: Blad execl pliku passenger");
        exit(EXIT_FAILURE);
    }

    station_master(data, MAX_TRAINS);

    cleanup_shared_memory(shmid, data); // czyszczenie pamieci dzielonej
    return 0;
}