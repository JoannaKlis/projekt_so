#include "functions.h"
#include "train_manager.h"
#include "signal.h"

int main() 
{
    setbuf(stdout, NULL);
    int memory;
    Data *data = NULL;

    shared_memory_create(&memory);
    shared_memory_address(memory, &data);

    signal(SIGCONT, handle_continue); // wznowienie sygnalu

    int sem_train_entry = semaphore_create(SEM_KEY_TRAIN_ENTRY); // semafor na wjazd na peron
    int sem_passengers_entry = semaphore_create(SEM_KEY_PASSENGERS_ENTRY);
    int sem_bike_entry = semaphore_create(SEM_KEY_BIKE_ENTRY);

    int msgid = msgget(MSG_KEY, 0600);
    if (msgid == -1) 
    {
        handle_error("KIEROWNIK POCIAGU: Blad otwierania kolejki komunikatow");
    }

    struct TrainMessage msg; // pobranie nr pociagu
    receive_message(msgid,getpid(),&msg);
    int train_number = msg.train_number;

    printf(COLOR_CYAN "KIEROWNIK POCIAGU (PID %d): Zarzadzam pociagiem numer %d\n" COLOR_RESET, getpid(), train_number);

    while (1) 
    {
        printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d czeka na zgode na wjazd na peron.\n" COLOR_RESET, train_number);
        
        //Wysyła wiadomość że chce wjechać
        msg.train_number = train_number;
        msg.mtype = 1;
        send_message(msgid,&msg);

        //Czeka na zezwolenie na wjazd
        receive_message(msgid,train_number+10,&msg);

        printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d wjezdza na peron.\n" COLOR_RESET, train_number);

        handle_passenger(data, sem_passengers_entry, sem_bike_entry); // obsluga pasazerow

        printf(COLOR_YELLOW "KIEROWNIK POCIAGU: Pociag %d opuszcza peron.\n" COLOR_RESET, train_number);

        // Wysyła wiadomość że pojechał
        msg.train_number = train_number; // wyslanie wiadomosci z nr pociagu
        msg.mtype = 2;
        send_message(msgid,&msg);

        sleep(TRAIN_DEPARTURE_TIME); // czas dotarcia na stacje 2
    }

    shared_memory_detach(data);
    semaphore_remove(sem_passengers_entry);
    semaphore_remove(sem_bike_entry);
    semaphore_remove(sem_train_entry);

    return 0;
}