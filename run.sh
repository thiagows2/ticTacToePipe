#!/bin/sh

gcc jogo_da_velha.c -o jogo_da_velha -lpthread
./jogo_da_velha
rm ./jogo_da_velha
