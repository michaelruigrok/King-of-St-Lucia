#include "koslShared.h"
#include "hub.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define WINNER 'W'

//A pointer to the players struct 
//Solely for SIGINT handler
static struct Players* playersSigintCopy;

int main(int argc, char** argv) {
    if (argc > 26 + 3 || argc < 5) {
        hub_quit(1);
    }

    struct Game game;

    char* notAllDigits = 0;
    game.pointsToWin = strtol(argv[2], &notAllDigits, 10);
    if (game.pointsToWin < 1 || *notAllDigits) {
        hub_quit(2);
    }

    load_rollfile(&game, argv[1]);

    struct Players players;
    playersSigintCopy = &players;
    init_players(&players, argc - 3, argv + 3);
    init_signal_handlers();
    game.playerCount = players.playerCount;
    game.inStLucia = players.playerCount;
    game_loop(&game, &players);
    return 0;
}

/*
 * load rolls from specified file into the game struct
 */
void load_rollfile(struct Game* game, char* filename) {
    FILE* rollFile = fopen(filename, "r");
    if (rollFile == NULL) {
        hub_quit(3);
    }

    fseek(rollFile, 0L, SEEK_END);
    char* rolls = malloc(sizeof(char) * ftell(rollFile));
    rewind(rollFile);

    int rollCount = 0;
    char temp = 0; //the character currently being evaluated
    while ((temp = fgetc(rollFile)) != EOF) {
        if ((temp >= '1' && temp <= '3') || temp == 'H' || 
                temp == 'A' || temp == 'P') {
            rolls[rollCount++] = temp;

        } else if (temp != '\n' && temp != 127) {
            hub_quit(4);
        }
    }

    if (rollCount == 0) {
        hub_quit(4);
    }

    game->rolls = rolls;
    game->rollCount = rollCount;
    game->nextRoll = 0;
    fclose(rollFile);
}

/*
 * return a series of rolls, the amount of which is specified as rollsNumber
 */
char* get_rolls(struct Game* game, char* rolls, int rollsNumber) {

    int rollsLeft = game->rollCount - game->nextRoll;

    //if there's enough space on the end, copy, otherwise return to the start
    if (rollsLeft > rollsNumber) {
        strncpy(rolls, game->rolls + game->nextRoll, rollsNumber);
        game->nextRoll += rollsNumber;
    } else {
        strncpy(rolls, game->rolls + game->nextRoll, rollsLeft);
        strncpy(rolls + rollsLeft, game->rolls, rollsNumber - rollsLeft);
        game->nextRoll = rollsNumber - rollsLeft;
    }
    qsort(rolls, rollsNumber, sizeof(char), dice_sort);
    return rolls;
}

/* 
 * return the contents of a player's response 
 */
char* get_response(struct Players* players, int player, char* response,
        int responseLength) {
    fflush(stdout);

    if (read(players->descriptors[player][0], response, responseLength) ==
            -1) {
        int waitStatus;
        waitpid(-1, &waitStatus, WNOHANG);
        if (WIFEXITED(waitStatus)) {
            hub_quit(5);
        }
    }

    //replace first newline with null character
    for (char* i = response; i < i + responseLength && *i != 0; i++) {
        if (*i == '\n') {
            *i = 0;
            break;
        }
    }
    return response;
}

/* 
 * return the first character of a player's response 
 */
char get_response_char(struct Players* players, int player) {
    char input[16];
    //char response;
    read(players->descriptors[player][0], input, 16);
    return input[0];
}

/* 
 * send a message to a player
 */
void inform_player(struct Players* players, int player, char* message) {
    if (!players->eliminated[player]) {
        dprintf(players->descriptors[player][1], "%s\n", message);
    }
}

/*
 * send a message to all players
 */
void broadcast(struct Players* players, char* message) {
    for (int player = 0; player < players->playerCount; player++) {
        inform_player(players, player, message);
    }

}

/*
 * initialise player processes & their attributes
 */
void init_players(struct Players* players, uint8_t playerCount,
        char** playerList) {
    int informPipe[2], respondPipe[2];
    int null = open("/dev/null", O_WRONLY);
    //update player count and allocate process ID storage
    players->playerCount = playerCount;
    players->processIds = malloc(sizeof(int) * playerCount);
    players->descriptors = malloc(sizeof(int*) * playerCount);

    for (int player = 0; player < playerCount; player++) {
        pipe(informPipe);
        pipe(respondPipe);

        int processId = fork();
        if (processId) {
            players->processIds[player] = processId;

            close(informPipe[0]);
            close(respondPipe[1]);

            players->descriptors[player] = malloc(sizeof(int) * 2);
            players->descriptors[player][0] = respondPipe[0];
            players->descriptors[player][1] = informPipe[1];

            check_player_init(players, player);

            close(informPipe[0]);
            close(respondPipe[1]);

        } else { 
            close(informPipe[1]);
            close(respondPipe[0]);

            dup2(null, 2);
            dup2(informPipe[0], 0);
            dup2(respondPipe[1], 1);

            char label[2];
            label[0] = (char)player + 'A';
            label[1] = 0;

            setsid();
            char playerCountString[3];
            sprintf(playerCountString, "%d", playerCount);
            execl(playerList[player], playerList[player], playerCountString,
                    label, NULL);
            hub_quit(5);
        }
    }
    init_player_properties(players);
}

/*
 * ensure player responds to hub, otherwise exit
 */
void check_player_init(struct Players* players, int player) {
    if (get_response_char(players, player) != '!') {
        int waitStatus;
        waitpid(-1, &waitStatus, WNOHANG);

        if (WIFEXITED(waitStatus)) {
            hub_quit(5);
        }
    }
}

/*
 * Initialise the health, tokens, score, and eliminated values
 * for each player
 */
void init_player_properties(struct Players* players) {
    players->health = malloc(sizeof(int8_t) * players->playerCount);
    memset(players->health, 10, sizeof(int8_t) * players->playerCount);

    players->tokens = malloc(sizeof(uint8_t) * players->playerCount);
    memset(players->tokens, 0, sizeof(uint8_t) * players->playerCount);

    players->score = malloc(sizeof(int) * players->playerCount);
    memset(players->score, 0, sizeof(int) * players->playerCount);

    players->eliminated = malloc(sizeof(bool) * players->playerCount);
    memset(players->eliminated, false, sizeof(bool) * players->playerCount);

}

/*
 * Enact the main body of the game
 */
void game_loop(struct Game* game, struct Players* players) {

    uint8_t player = 0;
    char rolls[6];

    while (true) {

        if (players->eliminated[player]) {
            player = (player + 1 == players->playerCount ? 0 : player + 1);
            continue;
        }

        manage_rerolls(game, players, player, rolls);

        evaluate_health_rolls(game, players, player, rolls);

        int score = 0;
        if (player == game->inStLucia) {
            score += 2;
        }

        evaluate_attack_rolls(game, players, player, rolls);

        if (!score && player == game->inStLucia) {
            score += 1;
        }

        if (score += evaluate_score_rolls(game, players, player, rolls)) {
            fprintf(stderr, "Player %c scored %d for a total of %d\n",
                    player + 'A', score, players->score[player] + score);

            char scoreMessage[12];
            sprintf(scoreMessage, "points %c %d", player + 'A', score);
            broadcast(players, scoreMessage);
            players->score[player] += score;
        }

        win_check(game, players);
        player = (player + 1 == players->playerCount ? 0 : player + 1);
    }
}

/*
 * created the final roll (after rerolls) for the given player
 *
 * returns rolls
 */
char* manage_rerolls(struct Game* game, struct Players* players, int player, 
        char* rolls) {

    get_rolls(game, rolls, 6);

    char message[16];
    char response[14];
    char reroll; //rerolls are calculated one at a time as necessary

    sprintf(message, "turn %s", rolls);
    inform_player(players, player, message);

    for (int rerolled = 0; rerolled < 3; rerolled++) {

        get_response(players, player, response, 14);

        if (!strncmp(response, "keepall", 7)) {
            break;
        } else if (!strncmp(response, "reroll ", 6)) {

            for (char* oldRoll = response + 7;
                    *oldRoll != 0 && oldRoll < response + 13; oldRoll++) {
                for (char* roll = rolls; roll < rolls + 6; roll++) {
                    if (*oldRoll == *roll) {
                        *roll = *get_rolls(game, &reroll, 1);
                        break;
                    }
                }
            }
            qsort(rolls, 6, sizeof(char), dice_sort);
            sprintf(message, "rerolled %s", rolls);
            inform_player(players, player, message);
        } else {
            //hub_quit(7);
        }
    }

    sprintf(message, "rolled %c %s", player + 'A', rolls);
    for (int recipient = 0; recipient < players->playerCount; recipient++) {
        if (recipient != player) {
            inform_player(players, recipient, message);
        }
    }
    fprintf(stderr, "Player %c rolled %s\n", player + 'A', rolls);
    fflush(stderr);
    return rolls;
}

/*
 * evaluate and report on the score gained by a player
 */
int evaluate_score_rolls(struct Game* game, struct Players* players,
        int player, char* rolls) {

    int score = 0;
    int toAdd; //the intermittent value to be added to the player

    if ((toAdd = count_rolls(rolls, 6, '1')) > 2) {
        score += toAdd - 2;
    }

    if ((toAdd = count_rolls(rolls, 6, '2')) > 2) {
        score += 2 + (toAdd - 3);
    }

    if ((toAdd = count_rolls(rolls, 6, '3')) > 2) {
        score += 3 + (toAdd - 3);
    }

    players->tokens[player] += count_rolls(rolls, 6, 'P');
    if (players->tokens[player] >= 10) {
        players->tokens[player] -= 10;
        score++;
    }


    return score;
}

/*
 * evaluate and apply health regained by a player from its roll
 */
void evaluate_health_rolls(struct Game* game, struct Players* players,
        int player, char* rolls) {

    int toHeal = count_rolls(rolls, 6, 'H');
    if (toHeal) {

        if (player != game->inStLucia) {
            if ((10 - (players->health[player])) < toHeal) {
                fprintf(stderr, "Player %c healed %d, health is now 10\n", 
                        player + 'A', 10 - players->health[player]);
                players->health[player] = 10;
            } else {
                players->health[player] += toHeal;
                fprintf(stderr, "Player %c healed %d, health is now %d\n", 
                        player + 'A', toHeal, players->health[player]);
            }

            /*
               } else {
               fprintf(stderr, "Player %c healed %d, health is now %d\n", 
               player + 'A', 0, players->health[player]);
               */
        }
    }
}

/*
 * evaluate and apply damage done by a player from their roll
 */
void evaluate_attack_rolls(struct Game* game, struct Players* players,
        int player, char* rolls) {

    int damage = count_rolls(rolls, 6, 'A');
    if (!damage) {
        return;
    }

    char damageAlert[15];

    if (player == game->inStLucia) {
        sprintf(damageAlert, "attacks %c %d out", player + 'A', damage);
        broadcast(players, damageAlert);

        for (int otherPlayer = 0; otherPlayer < players->playerCount;
                otherPlayer++) {
            if (otherPlayer != player) {
                damage_player(players, otherPlayer, damage);
            }
        }
        return;
    }


    if (game->inStLucia != players->playerCount) {
        sprintf(damageAlert, "attacks %c %d in", player + 'A', damage);
        broadcast(players, damageAlert);
    }

    //if:
    // there is no St Lucia Player
    // the given damage eliminated him
    // or he decided to leave
    if (game->inStLucia == players->playerCount ||
            damage_player(players, game->inStLucia, damage) ||
            ask_if_leaving(game, players, game->inStLucia)) {

        game->inStLucia = player; //damaging player takes St Lucia
        char message[7];
        sprintf(message, "claim %c", player + 'A');
        broadcast(players, message);
        fprintf(stderr, "Player %c claimed StLucia\n", player + 'A');
    }

}

/*
 * apply damage to the given player, and return if they died
 */
bool damage_player(struct Players* players, int player, int damage) {
    if (!players->health[player]) {
        return true;
    }

    if (players->health[player] - damage < 1) {
        fprintf(stderr, "Player %c took %d damage, health is now %d\n", 
                player + 'A', players->health[player], 0);
        players->health[player] = 0;
        return true;
    }
    players->health[player] -= damage;
    fprintf(stderr, "Player %c took %d damage, health is now %d\n", 
            player + 'A', damage, players->health[player]);
    return false;
}

/*
 * remove the attacked St Lucia player if they wish to go
 */
bool ask_if_leaving(struct Game* game, struct Players* players, int player) {
    fflush(stdout);

    if (player == players->playerCount) {
        return true;
    }
    char response[6];
    inform_player(players, player, "stay?");
    get_response(players, player, response, 6);

    if (!strcmp(response, "go")) {
        game->inStLucia = players->playerCount;
        return true;

    } else if (!strcmp(response, "stay")) {
        return false;
    }
    hub_quit(7);
    return false;
}

/*
 * Test to see if a winstate has been achieved and act accordingly
 */
void win_check(struct Game* game, struct Players* players) {

    for (int player = 0; player < players->playerCount; player++) {
        // check if player should be eliminated
        if (players->health[player] < 1 && !players->eliminated[player]) {
            char message[13];
            sprintf(message, "eliminated %c", player + 'A');
            broadcast(players, message);
            disable_signal_handlers();
            sleep(2);
            players->eliminated[player] = true;
            close(players->descriptors[player][1]);
            close(players->descriptors[player][0]);
            kill_child(players, player);
            init_signal_handlers();
            if (player == game->inStLucia) {
                game->inStLucia = players->playerCount;
            }
            players->eliminated[player] = true;
        }
    }

    for (int player = 0; player < players->playerCount; player++) {
        // check if player reaced points to win
        if (players->score[player] >= game->pointsToWin) {
            winner(players, player);
        }

    }
    //check if only one player remains
    int notEliminated = players->playerCount; // true if !(1 player left)
    for (int player = 0; player < players->playerCount; player++) {

        if (!players->eliminated[player]) {
            if (notEliminated != players->playerCount) {
                //two or more players exist
                notEliminated = players->playerCount;
                break;
            }
            notEliminated = player;
        }
    }
    if (notEliminated != players->playerCount) {
        winner(players, notEliminated);
    }

}


/*
 * announce to players a winner exists, then exit
 */
void winner(struct Players* players, int winner) {
    char message[9];
    sprintf(message, "%s %c", "winner", winner + 'A');
    fprintf(stderr, "Player %c wins\n", winner + 'A');
    broadcast(players, message);
    shutdown(WINNER);
}

/*
 * initialises the signal handlers
 */
void init_signal_handlers(void) {
    struct sigaction sigintAction;
    sigintAction.sa_handler = shutdown;
    sigintAction.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sigintAction, NULL);

    struct sigaction sigpipeAction;
    sigpipeAction.sa_handler = shutdown;
    sigpipeAction.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sigpipeAction, NULL);
}

/*
 * disable signal handlers
 */
void disable_signal_handlers(void) {
    sigaction(SIGPIPE, NULL, NULL);
    sigaction(SIGINT, NULL, NULL);

}

/*
 * gracefully shuts down the game
 */
void shutdown(int handler) {
    disable_signal_handlers();

    if (handler == SIGINT) {
        broadcast(playersSigintCopy, "shutdown");
        sleep(2);
        kill_children(playersSigintCopy);
        hub_quit(9);
    } else if (handler == SIGPIPE) {
        sleep(2);
        kill_children(playersSigintCopy);
        hub_quit(6);
    } else if (handler == WINNER) {
        sleep(2);
        kill_children(playersSigintCopy);
        exit(0);
    }

}

/*
 * kill a child, and report on their reaping
 */
void kill_child(struct Players* players, int player) {

    int waitStatus;
    int processId = playersSigintCopy->processIds[player];
    if (!players->eliminated[player]) {
        close(players->descriptors[player][1]);
    }


    if (waitpid(processId, &waitStatus, WNOHANG)) {
        if (WIFEXITED(waitStatus)) {
            if (WEXITSTATUS(waitStatus)) {
                printf("Player %c exited with status %d\n", player + 'A',
                        WEXITSTATUS(waitStatus));
            }

        } else if (!WIFSIGNALED(waitStatus)) {
            waitpid(processId, &waitStatus, WNOHANG);
            if (WIFSIGNALED(waitStatus)) {
                kill(processId, SIGKILL);
                printf("Player %c terminated due to signal %d\n", 
                        player + 'A', WTERMSIG(waitStatus));
            }
        }
    } else {
        kill(processId, SIGKILL);
        waitpid(processId, &waitStatus, 0);
        printf("Player %c terminated due to signal %d\n",
                player + 'A', WTERMSIG(waitStatus));
    }
}

/*
 * Leave no children alive, and report on their reaping
 */
void kill_children(struct Players* players) {
    for (int player = 0; player < playersSigintCopy->playerCount; player++) {
        kill_child(players, player);
    }
}

/*
 * prints an error message corresponding to the given value,
 * and exits using that value as an exit status
 */
void hub_quit(int exitValue) {
    //while(1);
    switch (exitValue) {
        case 1:
            fprintf(stderr, "%s%s", "Usage: stlucia rollfile winscore ", 
                    "prog1 prog2 [prog3 [prog4]]\n");
            exit(1);
        case 2:
            fprintf(stderr, "Invalid score\n");
            exit(2);
        case 3:
            fprintf(stderr, "Unable to access rollfile\n");
            exit(3);
        case 4:
            fprintf(stderr, "Error reading rolls\n");
            exit(4);
        case 5:
            fprintf(stderr, "Unable to start subprocess\n");
            exit(5);
        case 6:
            fprintf(stderr, "Player quit\n");
            exit(6);
        case 7:
            fprintf(stderr, "Invalid message received from player\n");
            exit(7);
        case 8:
            fprintf(stderr, "Invalid request by player\n");
            exit(8);
        case 9:
            fprintf(stderr, "SIGINT caught\n");
            exit(9);
    }
}
