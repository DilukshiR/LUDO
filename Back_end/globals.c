#include "Type.h"

int mystryCell = -1;
int Round = 0;
int currentplayer = 0;
int additionalRoll = 0;
int winner = -1;
int mysterySpawnRound = -1;

struct piece R1, R2, R3, R4;
struct piece Y1, Y2, Y3, Y4;
struct piece G1, G2, G3, G4;
struct piece B1, B2, B3, B4;
struct player Yellow, Blue, Red, Green;

struct piece *pieces[16] = {&Y1, &Y2, &Y3, &Y4,
                            &B1, &B2, &B3, &B4,
                            &R1, &R2, &R3, &R4,
                            &G1, &G2, &G3, &G4};

struct player *players[4] = {&Yellow, &Blue, &Red, &Green};
char Turn[4] = {'Y', 'B', 'R', 'G'};
