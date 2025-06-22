#!/bin/sh
g++ impl.cpp -DRED -c -o red.o -I.               
g++ impl.cpp -DBLUE -c -o blue.o -I.               
g++ executor.cpp red.o blue.o lib/game_controller.cpp lib/game_utils.cpp lib/types.cpp -o exec.exe