#include "functions.h"
#include "station_master.h"
#include "signal.h"

int main()
{
    setbuf(stdout, NULL);
    int memory;
    Data *data = NULL;

    size_t shared_memory_size = sizeof(Data); // obliczanie rozmiaru pamieci dzielonej

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY);
    int sem_passengers_entry = semaphore_create(SEM_KEY_PASSENGERS_ENTRY);
    int sem_passengers_exit = semaphore_create(SEM_KEY_PASSENGERS_EXIT);
    int sem_bike_entry = semaphore_create(SEM_KEY_BIKE_ENTRY);
    int sem_bike_exit = semaphore_create(SEM_KEY_BIKE_EXIT);

    if (semctl(sem_passengers_entry, 0, SETVAL, 0) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla wejścia pasazerow");
    }
    if (semctl(sem_passengers_exit, 0, SETVAL, 0) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla wyjścia pasazerow");
    }
    if (semctl(sem_bike_entry, 0, SETVAL, 0) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla wejścia pasazerow z rowerami");
    }
    if (semctl(sem_bike_exit, 0, SETVAL, 0) == -1) 
    {
        handle_error("ZARZADCA: Blad inicjalizacji semafora dla wyjścia pasazerów z rowerami");
    }


    data->current_train = 0; // reset wyboru pociagu
    data->free_seat = 0; // reset wolnych miejsc
    data->free_bike_spots = MAX_BIKES; // reset miejsc na rowery
    data->passengers_waiting = 0; // ustawienie poczatkowej liczby czekajacych pasazerow
    data->generating = 1; // pasazerowie sa generowani

    int msgid = msgget(MSG_KEY, IPC_CREAT | 0600);
    if (msgid == -1) 
    {
        handle_error("ZARZADCA: Blad tworzenia kolejki komunikatow");
    }
    
    pid_t train_manager_pids[MAX_TRAINS]; // generowanie kierownikow pociagow, po jednym dla kazdego pociagu

    for (int train = 0; train < MAX_TRAINS; train++) 
    {
        pid_t train_manager_pid = fork();
        if (train_manager_pid < 0) 
        {
            handle_error("ZARZADCA: Blad fork dla train_manager");
        }
        if (train_manager_pid == 0) // proces potomny
        {
            if (execl("./train_manager", "./train_manager", NULL) == -1) 
            {
                handle_error("ZARZADCA: Blad execl pliku train_manager");
            }
        }

        struct TrainMessage msg; // komunikat o danym numerze pociagu do kierownika
        msg.mtype = train_manager_pid; // PID kierownika jako typ wiadomości
        msg.train_number = train;
        if (msgsnd(msgid, &msg, sizeof(msg.train_number), 0) == -1) 
        {
            handle_error("ZARZADCA: Blad wysylania numeru pociagu");
        }

        train_manager_pids[train] = train_manager_pid;
    }



    pid_t passenger_pid; // deklaracja pidu pasazera

    int i = 0;
    data->generating = 1; // flaga - generowanie rozpoczete

    while (i < MAX_PASSENGERS_GENERATE && running)
    {
        passenger_pid = fork();

        if (passenger_pid < 0) // blad w fork
        {
            handle_error("ZARZADCA: Blad fork dla passenger");
        }
        if (passenger_pid == 0) // kod w procesie potomnym
        {
            if (execl("./passenger", "./passenger", NULL) == -1) // sprawdzenie bledu execl
            {
                handle_error("ZARZADCA: Blad execl pliku passenger");
            }
        }
        usleep(BLOCK_SLEEP * 100000);
        i++;
    }
    data->generating = 0; // koniec generowania

    station_master(data, sem_passengers_entry, sem_bike_entry, msgid, sem_train_entry); // uruchomienie dzialania zarzadcy stacji

    // oczekiwanie na zakonczenie procesow potomnych
    // wait_for_child_process(passenger_pid, "passenger");
    // wait_for_child_process(train_manager_pid, "train_manager");

    if (msgctl(msgid, IPC_RMID, NULL) == -1) 
    {
        handle_error("ZARZADCA: Blad usuwania kolejki komunikatow");
    }

    shared_memory_detach(data); // zwolnienie pamieci dzielonej
    shared_memory_remove(memory); // zwolnienie semaforow

    return 0;
}