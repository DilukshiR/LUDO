#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "server.h"
#include "Type.h"
#include "Game_logic.h"

#define LOG_MAX 20
#define LOG_LEN 200
#define BUFFER_SIZE 8192

typedef struct {
    int lastDice;
    int paused;
    int startPlayer;
    char logBuf[LOG_MAX][LOG_LEN];
    int logCount;
    int logStart;
} GameServer;

static GameServer server = {0};

static void log_add(const char *fmt, ...){
    char msg[LOG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    int idx = (server.logStart + server.logCount) % LOG_MAX;
    strncpy(server.logBuf[idx], msg, LOG_LEN - 1);
    server.logBuf[idx][LOG_LEN - 1] = '\0';
    if (server.logCount < LOG_MAX) {
        server.logCount++;
    } else {
        server.logStart = (server.logStart + 1) % LOG_MAX;
    }
}

static int can_move_piece(int currentPlayer, int rolledNumber, int pieceIndex){
    if (pieceIndex < 0 || pieceIndex > 3) return 0;
    struct piece *piece = pieces[currentPlayer * 4 + pieceIndex];
    if (piece->isBriefing) return 0;
    if (piece->position == -1) return rolledNumber == 6;
    if (piece->isHomeStraight){
        int adjusted = teleportingPower(currentPlayer, pieceIndex, rolledNumber);
        return (piece->homeStraight + adjusted <= 6);
    }
    return 1;
}

static int any_move_available(int currentPlayer, int rolledNumber){
    for (int i = 0; i < 4; i++){
        if (can_move_piece(currentPlayer, rolledNumber, i)) return 1;
    }
    return 0;
}

static int mystery_turns_left(){
    if (mysterySpawnRound < 0) return 0;
    int left = (mysterySpawnRound + 4) - Round;
    return left > 0 ? left : 0;
}

static void update_mystery_expiry(){
    if (mystery_turns_left() == 0){
        mystryCell = -1;
        mysterySpawnRound = -1;
    }
}

static void end_turn(int rolledNumber){
    if (currentplayer == (server.startPlayer + 3) % 4 && rolledNumber != 6){
        Round++;
        if (Round >= 2){
            mystryCellGenerator();
        }
    }
    update_mystery_expiry();
    handleTurn(rolledNumber);
}

static void game_init(){
    srand((unsigned)time(NULL));
    initializePieces();
    initializePlayers();

    winner = -1;
    Round = 0;
    mystryCell = -1;
    mysterySpawnRound = -1;
    additionalRoll = 0;

    server.lastDice = 0;
    server.paused = 0;
    server.logCount = 0;
    server.logStart = 0;

    int highestRoll = 0;
    for (int i = 0; i < 4; i++){
        int roll = rollDice();
        if (roll > highestRoll){
            highestRoll = roll;
            server.startPlayer = i;
        }
    }

    currentplayer = server.startPlayer;
    log_add("Game init. Player %d starts.", currentplayer + 1);
}

static void auto_step(){
    if (server.paused || winner != -1) return;

    int rolled = rollDice();
    server.lastDice = rolled;
    log_add("P%d rolled %d", currentplayer + 1, rolled);

    switch (currentplayer){
        case 0: yellowPlayer(rolled); break;
        case 1: bluePlayer(rolled); break;
        case 2: redPlayer(rolled); break;
        case 3: greenPlayer(rolled); break;
    }

    handleBriefingRound(currentplayer);
    end_turn(rolled);
}

static void handle_pause(){
    if (server.paused){
        log_add("Already paused");
        return;
    }
    server.paused = 1;
    log_add("Paused");
}

static void handle_resume(){
    if (!server.paused){
        log_add("Already running");
        return;
    }
    server.paused = 0;
    log_add("Resumed");
}

static void write_state_json(char *out, size_t max){
    int mleft = mystery_turns_left();
    int mcell = (mleft > 0) ? mystryCell : -1;

    size_t used = 0;
    used += snprintf(out + used, max - used,
        "{\"current_player\":%d,\"dice\":%d,\"round\":%d,\"paused\":%d,\"winner\":%d,"
        "\"mystery_cell\":%d,\"mystery_turns_left\":%d,\"pieces\":[",
        currentplayer, server.lastDice, Round, server.paused, winner, mcell, mleft);

    for (int p = 0; p < 4; p++){
        used += snprintf(out + used, max - used, "[");
        for (int k = 0; k < 4; k++){
            struct piece *pc = pieces[p * 4 + k];
            used += snprintf(out + used, max - used,
                "{\"position\":%d,\"homeStraight\":%d}%s",
                pc->position, pc->homeStraight, (k < 3) ? "," : "");
        }
        used += snprintf(out + used, max - used, "]%s", (p < 3) ? "," : "");
    }

    used += snprintf(out + used, max - used, "],\"log\":[");
    int count = server.logCount < 10 ? server.logCount : 10;
    for (int i = 0; i < count; i++){
        int idx = (server.logStart + server.logCount - count + i) % LOG_MAX;
        used += snprintf(out + used, max - used, "\"%s\"%s",
            server.logBuf[idx], (i < count - 1) ? "," : "");
    }
    used += snprintf(out + used, max - used, "]}");
}

static void send_response(int client, const char *status, const char *contentType, const char *body){
    char header[512];
    int len = (int)strlen(body);
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        status, contentType, len);
    send(client, header, h, 0);
    send(client, body, len, 0);
}

static void handle_client(int client){
    char req[BUFFER_SIZE];
    int n = recv(client, req, sizeof(req) - 1, 0);
    if (n <= 0) {
        close(client);
        return;
    }
    req[n] = '\0';

    char method[16], path[128];
    sscanf(req, "%15s %127s", method, path);

    if (strcmp(method, "OPTIONS") == 0){
        send_response(client, "204 No Content", "text/plain", "");
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/state") == 0){
        char body[BUFFER_SIZE];
        write_state_json(body, sizeof(body));
        send_response(client, "200 OK", "application/json", body);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/cmd") == 0){
        char *body = strstr(req, "\r\n\r\n");
        if (body) body += 4; else body = "";

        char cmd[128];
        strncpy(cmd, body, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';

        int len = (int)strlen(cmd);
        while (len > 0 && (cmd[len-1] == '\n' || cmd[len-1] == '\r' || cmd[len-1] == ' ')){
            cmd[--len] = '\0';
        }

        if (strncmp(cmd, "PAUSE", 5) == 0) {
            handle_pause();
        } else if (strncmp(cmd, "RESUME", 6) == 0) {
            handle_resume();
        } else if (strncmp(cmd, "RESET", 5) == 0) {
            game_init();
            log_add("Game reset");
        } else if (strncmp(cmd, "STEP", 4) == 0) {
            auto_step();
        } else {
            log_add("Unknown cmd: %s", cmd);
        }

        send_response(client, "200 OK", "text/plain", "OK");
    } else {
        send_response(client, "404 Not Found", "text/plain", "Not Found");
    }

    close(client);
}

void start_server(int port){
    printf("Initializing game...\n");
    game_init();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("socket");
        return;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        close(server_fd);
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0){
        perror("listen");
        close(server_fd);
        return;
    }

    printf("LUDO server running on http://localhost:%d\n", port);
    fflush(stdout);

    while (1){
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server_fd, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 250000; // 250ms tick

        int ready = select(server_fd + 1, &rfds, NULL, NULL, &tv);
        if (ready > 0 && FD_ISSET(server_fd, &rfds)){
            int client = accept(server_fd, NULL, NULL);
            if (client >= 0){
                handle_client(client);
            }
        }

        auto_step();
    }
}
