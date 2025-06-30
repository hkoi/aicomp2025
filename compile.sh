#!/bin/sh
g++ strategies/impl.cpp -std=c++20 -Wno-unused-result -DRED  -c -o strategies/red.o  -Ilib 
g++ strategies/impl.cpp -std=c++20 -Wno-unused-result -DBLUE -c -o strategies/blue.o -Ilib 
g++ lib/grader.cpp strategies/red.o strategies/blue.o lib/game_controller.cpp lib/types.cpp -std=c++20 -Wno-unused-result -o exec.exe
