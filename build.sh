#!/bin/bash
mkdir bin
gcc -o main main.c src/UDiskListener.c src/UDiskEventSocket.c src/UDiskEventClient.c src/EventParser.c -lpthread
mv main bin/main