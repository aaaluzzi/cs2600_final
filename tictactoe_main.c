#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

char board[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
int player1Turn = true;
int finished = false;

void printBoard() {
    puts("+-----------+");
    printf("| %c | %c | %c |\n", board[0], board[1], board[2]);
    puts("+-----------+");
    printf("| %c | %c | %c |\n", board[3], board[4], board[5]);
    puts("+-----------+");
    printf("| %c | %c | %c |\n", board[6], board[7], board[8]);
    puts("+-----------+");
}

bool validPosition(int boardIndex) {
    return boardIndex >= 0 && boardIndex <= 8 && board[boardIndex] == ' ';
}

int coordsToBoardIndex(int x, int y) {
    return (x - 1) + 3 * (y - 1);
}

bool won(char piece) {
    int winSum = 3 * piece;
    //horizontal lines
    for (int row = 0; row < 9; row += 3) {
        if (board[row] + board[row + 1] + board[row + 2] == winSum) {
            return true;
        }
    }
    //vertical lines
    for (int col = 0; col < 3; col++) {
        if (board[col] + board[col + 3] + board[col + 6] == winSum) {
            return true;
        }
    }
    //diagonal lines
    return board[0] + board[4] + board[8] == winSum || board[2] + board[4] + board[6] == winSum;
}

bool tie() {
    for (int i = 0; i < 9; i++) {
        if (board[i] == ' ') {
            return false;
        }
    }
    return true;
}

void startNextTurn(struct mosquitto *mosq) {
    printBoard();

    if (!player1Turn) {
        int boardIndex;
            int x, y;
            printf("Player 2, enter piece coords (q to quit):\n");
            do {
                do {
                    x = getchar() - '0';
                    if (x == 'q' - '0') {
                        finished = true;
                        return;
                    }
                } while (x < 0 || x > 3);
                do {
                    y = getchar() - '0';
                } while (y < 0 || y > 3);
                boardIndex = coordsToBoardIndex(x, y);
            } while (!validPosition(boardIndex));

        char pos[2];
        sprintf(pos, "%d", boardIndex);
        mosquitto_publish(mosq, NULL, "tictactoe/player2/move", strlen(pos), pos, 0, false);
    } else {
        puts("Waiting for Player 1 to go..");
    }
}

void onConnect(struct mosquitto *mosq, void *obj, int rc) {
    mosquitto_subscribe(mosq, NULL, "tictactoe/player1/input", rc);
    mosquitto_subscribe(mosq, NULL, "tictactoe/player1/move", rc);
    mosquitto_subscribe(mosq, NULL, "tictactoe/player2/move", rc);
}

void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    //printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
    if (strcmp(msg->topic, "tictactoe/player1/input") == 0) {
        if (strcmp((char *) msg->payload, "D") == 0) {
            printf("Player 1 forfeits!");
            finished = true;
            return;
        }
        int player1Pos = atoi(msg->payload) - 1;
        char posChar = player1Pos + '0';
        if (player1Turn && validPosition(player1Pos)) {
            mosquitto_publish(mosq, NULL, "tictactoe/player1/move", 8, &posChar, 0, false);
        }
    } else if (strcmp(msg->topic, "tictactoe/player1/move") == 0) {
        int player1Pos = atoi(msg->payload);
        board[player1Pos] = 'X';
        if (won('X')) {
            printBoard();
            puts("Player 1 wins!");
            finished = true;
        } else if (tie()) {
            printBoard();
            puts("Tie!");
            finished = true;
        } else {
            player1Turn = !player1Turn;
            startNextTurn(mosq);
        }
    } else if (strcmp(msg->topic, "tictactoe/player2/move") == 0) {
        int player2Pos = atoi(msg->payload);
        board[player2Pos] = 'O';
        player1Turn = !player1Turn;

        if (won('O')) {
            printBoard();
            puts("Player 2 wins!");
            finished = true;
        } else {
            startNextTurn(mosq);
        }
    }
}

int main() {
    int id = 12;

    mosquitto_lib_init();

    struct mosquitto *mosq;

    mosq = mosquitto_new("Laptop", true, &id);
    mosquitto_connect_callback_set(mosq, onConnect);
    mosquitto_message_callback_set(mosq, onMessage);

    mosquitto_connect(mosq, "localhost", 1883, 10);
    mosquitto_loop_start(mosq);
    puts("Start!");
    startNextTurn(mosq);
    while (!finished) {}

    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
