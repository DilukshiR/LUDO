#define Bhawana 9
#define Kotuwa 27
#define Pita_Kotuwa 46

extern int mystryCell;
extern int Round;
extern int currentplayer;
extern int additionalRoll;
extern int winner;
extern int mysterySpawnRound;

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
    int effectStartRound;  // Track when energy/sickness effects started
};

struct player {
    int sixCount;
    int playStatus;
    int approachCell;
    int place;
};

extern struct piece R1, R2, R3, R4;
extern struct piece Y1, Y2, Y3, Y4;
extern struct piece G1, G2, G3, G4;
extern struct piece B1, B2, B3, B4;
extern struct player Yellow, Blue, Red, Green;

extern struct piece *pieces[16];
extern struct player *players[4];
extern char Turn[4];