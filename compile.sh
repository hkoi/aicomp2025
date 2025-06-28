#!/bin/sh
g++ strategies/impl.cpp -DRED  -c -o strategies/red.o  -Ilib 
g++ strategies/impl.cpp -DBLUE -c -o strategies/blue.o -Ilib 
g++ lib/grader.cpp strategies/red.o strategies/blue.o lib/game_controller.cpp lib/game_utils.cpp lib/types.cpp -o exec.exe