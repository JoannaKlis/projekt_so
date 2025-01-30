#ifndef STATION_MASTER_H
#define STATION_MASTER_H
#include "signal.h"

void wait_for_child_process(pid_t pid, const char *process_name) // oczekiwanie na zakonczenie procesu potomnego
{
    printf(COLOR_CYAN "ZARZADCA: Oczekiwanie na zakonczenie procesu %s (PID: %d).\n" COLOR_RESET, process_name, pid);

    if (waitpid(pid, NULL, 0) == -1) // oczekiwanie na zakonczenie procesu za pomoca waitpid
    {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), COLOR_RED "ZARZADCA: Blad podczas oczekiwania na %s" COLOR_RESET, process_name);
        handle_error(error_message);
    }
    printf(COLOR_GREEN "ZARZADCA: Proces %s (PID: %d) zakonczony pomyslnie.\n" COLOR_RESET, process_name, pid);
}

void station_master(Data *data, int sem_passengers_entry, int sem_bike_entry, int msgid, int sem_train_entry)  // zarzadzanie pociagami
{
    TrainMessage msg;
    while (1)
    {
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, 0) == -1) // czekanie na powrot pociagu
        {
            perror("msgrcv");
            exit(1);
        }
        data->current_train=msg.train_number;

        semaphore_signal(sem_train_entry);

        printf(COLOR_CYAN "ZARZADCA: Pociag %d przyjechal na stacje 1.\n" COLOR_RESET, data->current_train + 1);


        printf(COLOR_CYAN "ZARZADCA: Liczba wszystkich oczekujacych: %d.\n" COLOR_RESET, data->passengers_waiting);

        if (signal2 == 0)
        {
            printf(COLOR_YELLOW "ZARZADCA: Sygnal blokady wejscia pasazerow (CTRL+L).\n" COLOR_RESET);
            sleep(BLOCK_SLEEP*1); // tymczasowa blokada wejsc
            signal2 = 1; // reset flagi signal2
        }

        sleep(BLOCK_SLEEP * 2); // oczekiwanie na wejscie pasazerow

        if (signal1 == 0)
        {
            printf(COLOR_YELLOW "ZARZADCA: Natychmiastowy odjazd pociagu wymuszony sygnalem (CTRL+K).\n" COLOR_RESET);
            signal1 = 1; // reset flagi signal1
        }
        else
        {
            printf(COLOR_CYAN "ZARZADCA: Pociag %d odjezdza ze stacji 1.\n" COLOR_RESET, data->current_train + 1);
        }

        semaphore_set_to_zero(sem_bike_entry); // blokada wejscia do pociagu dla pasazerow z rowerami
        printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow z rowerami zablokowane.\n" COLOR_RESET);

        semaphore_set_to_zero(sem_passengers_entry); // blokada wejscia do pociagu dla pasazerow bez rowerow
        printf(COLOR_RED "ZARZADCA: Wejscie dla pasazerow bez rowerow zablokowane.\n" COLOR_RESET);

        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 2, 0) == -1) // czekanie na powrot pociagu
        {
            perror("msgrcv");
            exit(1);
        }

        printf(COLOR_CYAN "ZARZADCA: Pociag %d pojechal\n" COLOR_RESET, data->current_train + 1);

        if (data->passengers_waiting == 0 && !data->generating) // sprawdzenie, czy brak oczekujacych pasazerow oraz czy zakonczono generowanie
        {
            printf(COLOR_BLUE "ZARZADCA: Nie ma oczekujacych pasazerow.\nKONIEC DZIALANIA.\n" COLOR_RESET);
            data->generating = -1; // flaga - zarzadca konczy dzialanie

            // odblokowanie semaforow na zakonczenie dzialania
            semaphore_signal(sem_passengers_entry);
            semaphore_signal(sem_bike_entry);
            break;
        }
    }
}

#endif // STATION_MASTER_H