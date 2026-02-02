#define Bhawana 9
#define Kotuwa 27
#define Pita_Kotuwa 46

int mystryCell;
int Round;
int currentplayer,additionalRoll=0;

struct piece {
    int position;
    char color[7];
    char Direction;
    int approachCellCount;
    int isHomeStraight;
    int homeStraight;
    int isEnergized;
    int isSick;
    int isBriefing;
    int briefingRound;
    int isCapturedPlayer;
};

struct player {
    int sixCount;
    int playStatus;
    int approachCell;
    int place;
};

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