#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Game_logic.h"

int main() {
    srand(time(NULL));
    playerDetails();
    initializePieces();
    initializePlayers();
    Play();
    return 0;
}

