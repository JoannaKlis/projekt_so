#!/bin/bash

gcc -o passenger passenger.c -lrt -lpthread
if [ $? -eq 0 ]; then
    echo "Kompilacja passenger zakonczona sukcesem."
else
    echo "Blad podczas kompilacji passenger." >&2
    exit 1
fi

gcc -o train_manager train_manager.c -lrt -lpthread
if [ $? -eq 0 ]; then
    echo "Kompilacja train_manager zakonczona sukcesem."
else
    echo "Blad podczas kompilacji train_manager." >&2
    exit 1
fi

gcc -o station_master station_master.c -lrt -lpthread
if [ $? -eq 0 ]; then
    echo "Kompilacja station_master zakonczona sukcesem."
else
    echo "Blad podczas kompilacji station_master." >&2
    exit 1
fi

echo "Wszystkie programy zostaly skompilowane pomyslnie."
