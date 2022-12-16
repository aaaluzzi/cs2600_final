#!/bin/bash
while true
do
    mosquitto_sub -h "localhost" -t "tictactoe/player1/input" | read -r payload;
    read -t 10 -p "Player 1 wants to play tic-tac-toe! Player 2, type fg then y if you want to play: ";
    if [[ -z "$REPLY" ]]
    then
        echo "Player 2 unavailable. Starting CPU game.."
        ./cpu.sh
    else
        echo "Starting game.."
        ./a.out
    fi
    sleep 5
done