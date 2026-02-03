#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Type.h"
#include "Game_logic.h"

//Display players name and name of Tokens
void playerDetails() {
    for (int j = 0; j < 4; j++) {
        printf("The %s player has four (04) pieces named %c1, %c2, %c3, and %c4\n",
               pieces[j * 4]->color, Turn[j], Turn[j], Turn[j], Turn[j]);
    }
    printf("\n");
}
//initializing  variables of pieces
void initializePieces() {
    for (int i = 0; i < 16; i++) {
        pieces[i]->position = -1;
        pieces[i]->homeStraight = 0;
        pieces[i]->approachCellCount = 0;
        pieces[i]->isHomeStraight = 0;
        pieces[i]->isEnergized = 0;
        pieces[i]->isSick = 0;
        pieces[i]->isBriefing = 0;
        pieces[i]->briefingRound=0;
        pieces[i]->isCapturedPlayer=0;
        pieces[i]->effectStartRound=0;

        if (i < 4) {
            strcpy(pieces[i]->color, "Yellow");
        } else if (i < 8) {
            strcpy(pieces[i]->color, "Blue");
        } else if (i < 12) {
            strcpy(pieces[i]->color, "Red");
        } else {
            strcpy(pieces[i]->color, "Green");
        }
    }
}
// initializing varibales of players
void initializePlayers() {
    for (int i = 0; i < 4; i++) {
        players[i]->playStatus = 1;
    }
    players[0]->approachCell = 0;
    players[1]->approachCell = 13;
    players[2]->approachCell = 26;
    players[3]->approachCell = 39;
}
//Handling players  Turns  
void handleTurn(int rolledNumber){
    if(rolledNumber==6 || additionalRoll==1){
        currentplayer=currentplayer;
        if(rolledNumber==6){
            players[currentplayer]->sixCount++;
            if( players[currentplayer]->sixCount==3){
                currentplayer=(currentplayer+1)%4;
                players[currentplayer]->sixCount=0;
            }
        }
    }
    else{
        currentplayer=(currentplayer+1)%4;
        if(players[(currentplayer+3)%4]->sixCount>0){
            players[(currentplayer+3)%4]->sixCount=0;
        }
    }
}
//simulate the dice roll
int rollDice() {
    return rand() % 6 + 1;
}
//select the starting player based on the rolls
int whoStartGame() {
    int highestRoll = 0;
    int startPlayer = 0;

    for (int i = 0; i < 4; i++) {
        int rolledNumber = rollDice();
        printf("%s rolled %d\n", pieces[i * 4]->color, rolledNumber);
        if (rolledNumber > highestRoll) {
            highestRoll = rolledNumber;
            startPlayer = i;
        }
    }
    printf("\n%s player has the highest roll and will begin the game\n", pieces[startPlayer * 4]->color);
    printf("The order of a single round is %s, %s, %s, %s\n\n",
           pieces[startPlayer * 4]->color,
           pieces[((startPlayer + 1) % 4) * 4]->color,
           pieces[((startPlayer + 2) % 4) * 4]->color,
           pieces[((startPlayer + 3) % 4) * 4]->color);
    return startPlayer;
}
//generate mystryCells in the random location every 4 rounds starting from round 3
void mystryCellGenerator(){
    if(Round==2 || (Round>2 && (Round-2)%4==0)){
        mystryCell=rand()%52;
        mysterySpawnRound=Round;
        printf("\n");
        printf("A mystery cell has spawned in location L%d and will be at this location for the next four rounds\n",mystryCell);
    }
}
//display the players details of each Round
void detailsOfEachRound(){
    int count;
    printf("\n");
    for(int i=0;i<4;i++){
        count=countPiecesOnBoard(i);
        printf("%s player has now %d/4 on the board and %d/4 on the base \n",pieces[i*4]->color,count,4-count);
        printf("=============================\n");
        printf("Location of Pieces %s\n",pieces[i*4]->color);
        printf("=============================\n");
        for(int j=0;j<4;j++){
            if(pieces[i*4+j]->position==-1){
                printf("piece %c%d -> base\n",Turn[i],j+1);
            }
            else if(pieces[i*4+j]->isHomeStraight && pieces[i*4+j]->homeStraight!=6){
                printf("piece %c%d -> homepath\n",Turn[i],j+1);
            }
            else if(pieces[i*4+j]->isHomeStraight && pieces[i*4+j]->homeStraight==6){
                printf("piece %c%d -> home\n",Turn[i],j+1);
            }
            else{
                printf("piece %c%d -> L%d \n",Turn[i],j+1,pieces[i*4+j]->position);
            }
        }
    }
    if(Round>2){
        printf("\n");
        printf("The mystrycell is at L%d and will be at the location for the next 4 rounds\n",mystryCell);
    }
    printf("\n");
}
//counting the token on the board of each player
int countPiecesOnBoard(int currentPlayer) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (pieces[currentPlayer * 4 + i]->position != -1) {
            count++;
        }
    }
    return count;
}
//check if a player can start piece 
int ableToStartPiece(int currentPlayer){
    int i;
    for(i=0;i<4;i++){
        if(pieces[currentPlayer*4+i]->position==-1){
            return i;
        }
    }
    return -1;
}
//moves a piece from base 
void startPiece(int currentPlayer,int pieceIndex){
    pieces[currentPlayer*4+pieceIndex]->position=players[currentPlayer]->approachCell+2;
    rotation(currentPlayer,pieceIndex,Toss());
    int count= countPiecesOnBoard(currentPlayer);
    printf("%s player moves piece %c%d to the starting point \n",pieces[currentPlayer*4]->color,Turn[currentPlayer],pieceIndex+1);
    printf("%s player has now %d/4 on the board and %d/4 on the base \n",pieces[currentPlayer*4]->color,count,4-count);
}
// check and select piece randomly to move on the board
int ableToMove(int currentPlayer){
    int count=0,index[4];
    for(int i=0;i<4;i++){
        if(pieces[currentPlayer*4+i]->position!= -1 && pieces[currentPlayer*4+i]->isHomeStraight==0 && pieces[currentPlayer*4+i]->isBriefing != 1 ){
            index[count]=i;
            count +=1;
        }
    }
    if(count>=2){
        return index[rand()%count];
    }
    else if(count==1){
        return index[0];
    }
    else{
        return -1;
    }
}
// simulate a coin toss
int Toss() {
    return rand() % 2;
}
//adjust the position of a piece on the board 
void adjustPosition(int currentPlayer,int pieceIndex){
    if(pieces[currentPlayer*4+pieceIndex]->Direction=='H'){
        if(pieces[currentPlayer*4+pieceIndex]->position>51){
            pieces[currentPlayer*4+pieceIndex]->position-=52;
        }
    }
    else{
        if(pieces[currentPlayer*4+pieceIndex]->position<0){
            pieces[currentPlayer*4+pieceIndex]->position+=52;
        }
    }
}
//set the direct of the piece 
void rotation(int currentPlayer, int pieceIndex, int toss) {
    if (toss == 1) {
        pieces[currentPlayer * 4 + pieceIndex]->Direction = 'H';
    } else {
        pieces[currentPlayer * 4 + pieceIndex]->Direction = 'T';
    }
}
//move piece on the board base on the direction and rolledNumber
void movePiece(int currentPlayer, int rolledNumber, int pieceIndex) {
    struct piece *piece = pieces[currentPlayer * 4 + pieceIndex];
    int pastLocation = piece->position;
    rolledNumber=teleportingPower(currentPlayer,pieceIndex,rolledNumber);
    if(piece->Direction == 'H'){
        for(int i=rolledNumber;i>0;i--){
            piece->position+=1;
            // Must have captured at least one opponent to enter home straight (LUDO-CS rule)
            if(piece->approachCellCount>=1 && piece->isCapturedPlayer==1){
                piece->position=-2;
                piece->isHomeStraight=1;
                piece->homeStraight++;
            }
            else if(piece->position==players[currentPlayer]->approachCell){
                piece->approachCellCount=1;
            }
            else if(piece->position==52){
                piece->position-=52;
            }
        }
        printf("%s moves piece %c%d from location L%d to L%d by %d units in clockwise\n",piece->color, Turn[currentPlayer], pieceIndex + 1,pastLocation, piece->position, rolledNumber);
    }
    else{
        for(int i=rolledNumber;i>0;i--){
            piece->position-=1;
            // Counter-clockwise: must pass approach cell twice before entering home straight (LUDO-CS rule)
            if(piece->approachCellCount>=2 && piece->isCapturedPlayer==1){
                    piece->position=-2;
                    piece->isHomeStraight=1;
                    piece->homeStraight+=1;
            }
            else if(piece->position==players[currentPlayer]->approachCell){
                piece->approachCellCount++;
            }
            else if(piece->position==-1){
                piece->position+=52;
            }
        }
        printf("%s moves piece %c%d from location L%d to L%d by %d units in counterclockwise\n", piece->color, Turn[currentPlayer], pieceIndex + 1,pastLocation, piece->position, rolledNumber);
    }
    if(piece->position==mystryCell){
        handleMysteryCell(currentPlayer,pieceIndex);
    }
}
//check if a piece can move on the homestraight
int ableToMoveInHomeStraight(int currentPlayer,int rolledNumber){
    int i;
    for(i=0;i<4;i++){
        int adjustedRoll=teleportingPower(currentPlayer,i,rolledNumber);
        if(pieces[currentPlayer*4+i]->isHomeStraight==1 && pieces[currentPlayer*4+i]->homeStraight+adjustedRoll <=6){
            return i;
        }
    }
    return -1;
}
//move piece in home path
void MoveInHomeStraight(int currentPlayer,int rolledNumber,int pieceIndex){
    int adjustedRoll=teleportingPower(currentPlayer,pieceIndex,rolledNumber);
    if(pieces[currentPlayer*4+pieceIndex]->homeStraight+adjustedRoll<=6){
        pieces[currentPlayer*4+pieceIndex]->homeStraight+=adjustedRoll;
        printf("Moving in homestraight\n");
        if(pieces[currentPlayer*4+pieceIndex]->homeStraight==6){
            printf("%c%d Reached Home\n",Turn[currentPlayer],pieceIndex+1);
            findWinner(currentPlayer);
        }
    }
}
//check for a winner
void findWinner(int currentPlayer){
    int homeCount=0;
    for(int i=0;i<4;i++){
        if(pieces[currentPlayer*4+i]->homeStraight==6){
            homeCount++;
        }
    }
    if(homeCount==4){
        players[currentPlayer]->playStatus=0;
        winner=currentPlayer;
        printf("%s player Wins!!\n",pieces[currentPlayer*4]->color);
    }
}
//check if a piece can capture opponent piece 
int ableToCapture(int currentPlayer,int rolledNumber){
    int position;
    for(int i=0;i<4;i++){
        rolledNumber=teleportingPower(currentPlayer,i,rolledNumber);
        position=pieces[currentPlayer*4+i]->position;
        if(pieces[i]->Direction=='H' && pieces[i]->position!=-2 && pieces[i]->position!=-1){
            for(int j=rolledNumber-1;j>0;j--){
                position++;
                if(position==players[currentPlayer]->approachCell){
                    continue;
                }
            }
            if(position>51){
                position-=52;
            }
            for(int j=0;j<16;j++){
                if(position==pieces[j]->position && strcmp(pieces[currentPlayer*4+i]->color,pieces[j]->color)!= 0 && position!=2 && position!=15 && position!=28 && position!=41 && position-rolledNumber!=-1 && pieces[j]->position!=-2){
                    return i;
                }
            }
        }
        else if(pieces[i]->position!=-2 && pieces[i]->position!=-1){
            position=pieces[currentPlayer*4+i]->position;
            for(int j=rolledNumber-1;j>0;j--){
                position--;
                if(position==players[currentPlayer]->approachCell){
                    continue;
                }
            }
            if(position<0){
                position+=52;
            }
            for(int j=0;j<16;j++){
                if(position==pieces[j]->position && strcmp(pieces[currentPlayer*4+i]->color,pieces[j]->color)!= 0 && position!=2 && position!=15 && position!=28 && position!=41 && position-rolledNumber!=-1 && pieces[j]->position!=-2){
                    return i;
                }
            }
        }
    }
    return -1;
}
// capture opponent player 
void capturePlayer(int rolledNumber, int currentPlayer,int pieceIndex) {
    rolledNumber=teleportingPower(currentPlayer,pieceIndex,rolledNumber);
    if(pieces[currentPlayer*4+pieceIndex]->Direction=='H'){
        for (int i = 0; i < 16; i++) {
            if (pieces[i]->position == pieces[currentPlayer*4+pieceIndex]->position+rolledNumber && strcmp(pieces[i]->color, pieces[currentPlayer * 4]->color) != 0) {
                pieces[currentPlayer*4+pieceIndex]->position+=rolledNumber;
                adjustPosition(currentPlayer,pieceIndex);
                printf("%s piece %c%d lands on square L%d, captures %s piece %c%d, and returns it to the base\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], pieceIndex + 1, pieces[currentPlayer*4+pieceIndex]->position, pieces[i]->color, Turn[(i / 4) % 4], (i % 4 + 1));
                if(pieces[currentPlayer*4+pieceIndex]->position==mystryCell){
                    handleMysteryCell(currentPlayer,pieceIndex);
                }
                pieces[i]->position = -1;
                pieces[i]->isCapturedPlayer=1;
                additionalRoll=1;
            }
        }
    }
    else{
        for (int i = 0; i < 16; i++) {
            if (pieces[i]->position == pieces[currentPlayer*4+pieceIndex]->position-rolledNumber && strcmp(pieces[i]->color, pieces[currentPlayer * 4]->color) != 0) {
                pieces[currentPlayer*4+pieceIndex]->position-=rolledNumber;
                adjustPosition(currentPlayer,pieceIndex);
                printf("%s piece %c%d lands on square L%d, captures %s piece %c%d, and returns it to the base\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], pieceIndex + 1, pieces[currentPlayer*4+pieceIndex]->position+rolledNumber, pieces[i]->color, Turn[(i / 4) % 4], (i % 4 + 1));

                if(pieces[currentPlayer*4+pieceIndex]->position==mystryCell){
                    handleMysteryCell(currentPlayer,pieceIndex);
                }
                pieces[i]->position = -1;
                pieces[i]->isCapturedPlayer=1;
                additionalRoll=1;
            }
        }
    }
}
//handle the briefing state and special effects of a piece 
void handleBriefingRound(int currentPlayer){
    for(int i=0;i<4;i++){
        if(pieces[currentPlayer*4+i]->isBriefing==1){
            pieces[currentPlayer*4+i]->briefingRound--;
            if(pieces[currentPlayer*4+i]->briefingRound==0){
                pieces[currentPlayer*4+i]->isBriefing=0;
            }
        }
        // Reset energized/sick effects after 4 rounds (LUDO-CS rule)
        if(pieces[currentPlayer*4+i]->isEnergized || pieces[currentPlayer*4+i]->isSick){
            if(Round - pieces[currentPlayer*4+i]->effectStartRound >= 4){
                pieces[currentPlayer*4+i]->isEnergized=0;
                pieces[currentPlayer*4+i]->isSick=0;
            }
        }
    }
}
//teleporting to special Cell when a piece lands on a mytery cell
void handleMysteryCell(int currentPlayer, int pieceIndex) {
    int teleport = rand()%6+1;
    switch (teleport) {
        case 1:
            pieces[currentPlayer * 4 + pieceIndex]->position = Bhawana;
            pieces[currentPlayer * 4 + pieceIndex]->isEnergized = rand() % 2; // 50% chance of being energized
            pieces[currentPlayer * 4 + pieceIndex]->isSick = !pieces[currentPlayer * 4 + pieceIndex]->isEnergized;
            pieces[currentPlayer * 4 + pieceIndex]->effectStartRound = Round;  // Track when effect started
            printf("%s's piece %c%d is teleported to Bhawana\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            break;
        case 2:
            pieces[currentPlayer * 4 + pieceIndex]->position = Kotuwa;
            pieces[currentPlayer * 4 + pieceIndex]->isBriefing = 1;
            pieces[currentPlayer * 4 + pieceIndex]->briefingRound = 4;
            printf("%s's piece %c%d is teleported to Kotuwa and will be in briefing for 4 rounds\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            break;
        case 3:
            pieces[currentPlayer * 4 + pieceIndex]->position = Pita_Kotuwa;
            if (pieces[currentPlayer * 4 + pieceIndex]->Direction == 'H') {
                pieces[currentPlayer * 4 + pieceIndex]->Direction = 'T';
                printf("%s's piece %c%d is teleported to Pita-Kotuwa and direction changed\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            } else {
                pieces[currentPlayer * 4 + pieceIndex]->position = Kotuwa;
                printf("%s's piece %c%d is teleported to Kotuwa (from Pita-Kotuwa)\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            }
            break;
        case 4:
            pieces[currentPlayer * 4 + pieceIndex]->position =-1;
            printf("%s's piece %c%d is teleported to Base\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            break;
        case 5:
            pieces[currentPlayer * 4 + pieceIndex]->position = players[currentPlayer]->approachCell + 2;
            printf("%s's piece %c%d is teleported to X of the piece color\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            break;
        case 6:
            pieces[currentPlayer * 4 + pieceIndex]->position = players[currentPlayer]->approachCell;
            pieces[currentPlayer*4+pieceIndex]->approachCellCount+=1;
            printf("%s's piece %c%d is teleported to Approach of the piece color\n", pieces[currentPlayer * 4]->color, Turn[currentPlayer], (pieceIndex + 1));
            break;
    }
}
//applies the effect of the mystryCell
int teleportingPower(int currentPlayer,int pieceIndex,int rolledNumber){
    if(pieces[currentPlayer*4+pieceIndex]->isEnergized){
        return rolledNumber*2;
    }
    else if(pieces[currentPlayer*4+pieceIndex]->isSick){
        return rolledNumber/2;
    }
    else{
        return rolledNumber;
    }
}
void redPlayer(int rolledNumber){
    int pieceIndex;
    if(rolledNumber==6){
        pieceIndex=ableToCapture(2,rolledNumber);
        if(pieceIndex>=0){
            capturePlayer(rolledNumber,2,pieceIndex);
        }
        else{
            pieceIndex=ableToStartPiece(2);

            if(pieceIndex>=0){
                startPiece(2,pieceIndex);
            }
            else{
                pieceIndex=ableToMoveInHomeStraight(2,rolledNumber);
                if(pieceIndex>=0){
                    MoveInHomeStraight(2,rolledNumber,pieceIndex);
                }
                else{
                    pieceIndex=ableToMove(2);
                    if(pieceIndex>=0){
                        movePiece(2,rolledNumber,pieceIndex);

                    }
                }
            }
        }
    }
    else{
        pieceIndex=ableToCapture(2,rolledNumber);
        if(pieceIndex>=0){
            capturePlayer(rolledNumber,2,pieceIndex);
        }
        else{
            pieceIndex=ableToMoveInHomeStraight(2,rolledNumber);
            if(pieceIndex>=0){
                MoveInHomeStraight(2,rolledNumber,pieceIndex);
            }
            else{
                pieceIndex=ableToMove(2);
                if(pieceIndex>=0){
                    movePiece(2,rolledNumber,pieceIndex);
                }
            }
        }
    }
}
void greenPlayer(int rolledNumber){
    int pieceIndex;
    if(rolledNumber==6){
        pieceIndex=ableToStartPiece(3);
        if(pieceIndex>=0){
            startPiece(3,pieceIndex);
        }
        else{
            pieceIndex=ableToMoveInHomeStraight(3,rolledNumber);
            if(pieceIndex>=0){
                MoveInHomeStraight(3,rolledNumber,pieceIndex);
            }
            else{
                pieceIndex=ableToMove(3);
                if(pieceIndex>=0){
                    movePiece(3,rolledNumber,pieceIndex);
                }
                else{
                    pieceIndex=ableToCapture(3,rolledNumber);
                    if(pieceIndex>=0){
                        capturePlayer(rolledNumber,3,pieceIndex);
                    }
                }
            }
        }
    }
    else{
        pieceIndex=ableToMoveInHomeStraight(3,rolledNumber);
        if(pieceIndex>=0){
            MoveInHomeStraight(3,rolledNumber,pieceIndex);
        }
        else{
            pieceIndex=ableToMove(3);
            if(pieceIndex>=0){
                movePiece(3,rolledNumber,pieceIndex);
            }
            else{
                pieceIndex=ableToCapture(3,rolledNumber);
                if(pieceIndex>=0){
                    capturePlayer(rolledNumber,3,pieceIndex);
                }
            }
        }
    }
}
void yellowPlayer(int rolledNumber){
    int pieceIndex;
    if(rolledNumber==6){
        pieceIndex=ableToStartPiece(0);
        if(pieceIndex>=0){
            startPiece(0,pieceIndex);
        }
        else{
            pieceIndex=ableToCapture(0,rolledNumber);
            if(pieceIndex>=0){
                capturePlayer(rolledNumber,0,pieceIndex);
            }
            else{
                pieceIndex=ableToMoveInHomeStraight(0,rolledNumber);
                if(pieceIndex>=0){
                MoveInHomeStraight(0,rolledNumber,pieceIndex);
                }
                else{
                    pieceIndex=ableToMove(0);
                    if(pieceIndex>=0){
                        movePiece(0,rolledNumber,pieceIndex);
                    }
                }
            }
        }
    }
    else{
        pieceIndex=ableToCapture(0,rolledNumber);
        if(pieceIndex>=0){
            capturePlayer(rolledNumber,0,pieceIndex);
        }
        else{
            pieceIndex=ableToMoveInHomeStraight(0,rolledNumber);
            if(pieceIndex>=0){
                MoveInHomeStraight(0,rolledNumber,pieceIndex);
            }
            else{
                pieceIndex=ableToMove(0);
                if(pieceIndex>=0){
                    movePiece(0,rolledNumber,pieceIndex);
                }
            }
        }
    }
}
int ableToMoveToMystryCell(int currentPlayer,int rolledNumber){
    int position;
    for(int i=0;i<4;i++){
        if(pieces[currentPlayer*4+i]->position==-1 || pieces[currentPlayer*4+i]->isHomeStraight==1) continue;
        
        position=pieces[currentPlayer*4+i]->position;
        if(pieces[currentPlayer*4+i]->Direction=='H'){
            for(int j=rolledNumber;j>0;j--){
                position++;
                if(position>51){
                    position-=52;
                }
            }
        }
        else{
            for(int j=rolledNumber;j>0;j--){
                position--;
                if(position<0){
                    position+=52;
                }
            }
        }
        if(position==mystryCell){
            return i;
        }
    }
    return -1;
}
void bluePlayer(int rolledNumber){
    int pieceIndex;
    if(rolledNumber==6){
        pieceIndex=ableToMoveToMystryCell(1,rolledNumber);
        if(pieceIndex>=0 && Round>2){
            for(int j=rolledNumber;j>0;j--){
                pieces[4+pieceIndex]->position++;
                if( pieces[4+pieceIndex]->position>51){
                    pieces[4+pieceIndex]->position-=52;
                }
            }
        }
        else{
            pieceIndex=ableToStartPiece(1);
            if(pieceIndex>=0){
                startPiece(1,pieceIndex);
            }
            else{
                pieceIndex=ableToCapture(1,rolledNumber);
                if(pieceIndex>=0){
                    capturePlayer(rolledNumber,1,pieceIndex);
                }
                else{
                    pieceIndex=ableToMove(1);
                    if(pieceIndex>=0){
                        movePiece(1,rolledNumber,pieceIndex);
                    }
                    else{
                        pieceIndex=ableToMoveInHomeStraight(1,rolledNumber);
                        if(pieceIndex>=0){
                            MoveInHomeStraight(1,rolledNumber,pieceIndex);
                        }
                    }
                }
            }
        }
    }
    else{
        pieceIndex=ableToMoveToMystryCell(1,rolledNumber);
        if(pieceIndex>=0 && Round>2){
            for(int j=rolledNumber;j>0;j++){
                pieces[4+pieceIndex]->position++;
                if( pieces[4+pieceIndex]->position>51){
                    pieces[4+pieceIndex]->position-=52;
                }
            }
        }
        else{
            pieceIndex=ableToCapture(1,rolledNumber);
            if(pieceIndex>=0){
                capturePlayer(rolledNumber,1,pieceIndex);
            }
            else{
                pieceIndex=ableToMove(1);
                if(pieceIndex>=0){
                    movePiece(1,rolledNumber,pieceIndex);
                }
                else{
                    pieceIndex=ableToMoveInHomeStraight(1,rolledNumber);
                    if(pieceIndex>=0){
                        MoveInHomeStraight(1,rolledNumber,pieceIndex);
                    }
                }
            }
        }
    }
}
void Play(){
    currentplayer=whoStartGame();
    int startPlayer=currentplayer;
    while(players[0]->playStatus==1 &&  players[1]->playStatus==1 &&  players[2]->playStatus==1 &&  players[3]->playStatus==1){
        int rolledNumber=rollDice();
        additionalRoll=0;
        switch(currentplayer){
            case 0:
                printf("Yellow player rolls %d\n",rolledNumber);
                yellowPlayer(rolledNumber);
                handleBriefingRound(0);
                printf("\n");
                break;
            case 1:
                printf("Blue player rolls %d\n",rolledNumber);
                bluePlayer(rolledNumber);
                handleBriefingRound(1);
                printf("\n");
                break;
            case 2:
                printf("Red player rolls %d\n",rolledNumber);
                redPlayer(rolledNumber);
                handleBriefingRound(2);
                printf("\n");
                break;
            case 3:
                printf("Green player rolls %d\n",rolledNumber);
                greenPlayer(rolledNumber);
                handleBriefingRound(3);
                printf("\n");
                break;
        }
        if(currentplayer==(startPlayer+3)%4 && rolledNumber!=6){
            Round++;
            if(Round>2){
                mystryCellGenerator();
            }
	    if(players[0]->playStatus==1 &&  players[1]->playStatus==1 &&  players[2]->playStatus==1 &&  players[3]->playStatus==1){
		    detailsOfEachRound();
		}
        }
        handleTurn(rolledNumber);
    }
}
