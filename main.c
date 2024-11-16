#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    printf("PID: %d\nPPID: %d\nUID: %d\nGID: %d\n", getpid(), getppid(), getuid(), getgid());

    exit(0);
}