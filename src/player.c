#include "koslShared.h"
#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char** argv) {
    struct Player player;
    player_init(&player, argc, argv);
    init_signal_handlers();
    char input[32];
    char temp[32];
    while(temp == fgets(temp, 32, stdin)) {
        strcpy(input, temp);
        fprintf(stderr, "From StLucia:%s", input);
        fflush(stderr);
        fflush(stdout);

        if (!input[0]) {
            continue;
        }
        parse_input(&player, input);

    }
    player_quit(4); //unexpected EOF 
    return 0;
}

/*
 * takes input from the hub and acts accordingly
 */
void parse_input(struct Player* player, char* input) {

    if (!strcmp(input, "shutdown\n")) {
        exit(0);
    } else if (/*strlen(input) <= 10 && */
            check_player_statement(player, input, "eliminated ", 11)) {
        if (player->label == input[11]) {
            exit(0);
        }
    } else if (check_player_statement(player, input, "winner ", 7)) {
        exit(0);
    } else if (strlen(input) == 12 && !strncmp(input, "turn ", 5)) {
        if (parse_rolls(player, input, 5)) {
            get_input(input, "rerolled ", 9);
            parse_rolls(player, input, 9);
        }
        printf("keepall\n");
        fflush(stdout);
    } else if (check_player_statement(player, input, "attacks ", 8) &&
            *valid_number(input + 10)) {
        update_health(player, input[8], input + 10);
    } else if (check_player_statement(player, input, "points ", 7) &&
            *valid_number(input + 9) == '\n') {
    } else if (check_player_statement(player, input, "claim ", 6)) {
        player->hubPlayer = input[6];
        /*(strlen(input) == 16 && */
    } else if (check_player_statement(player, input, "rerolled ", 8)) {
        evaluate_health_rolls(player, player->label, input + 9);
        /*strlen(input) == 16 && */
    } else if (check_player_statement(player, input, "rolled ", 7) &&
            valid_roll(input + 9, 6)) {
        evaluate_health_rolls(player, input[7], input + 9);
    } else if (feof(stdin)) {
        player_quit(4);
    } else if (!strncmp(input, "stay?", 5)) {
        if (decide_if_staying(player)) {
            //write(1, "stay\n", sizeof(char) * 3);
            printf("stay\n");
        } else {
            printf("go\n");
            //write(1, "go\n", sizeof(char) * 3);
        }
        fflush(stdout);
    } else {
        //fprintf(stderr, "receving %s, char %c, letter %d\n", input,
        //input[0], input[0]);
        player_quit(5);
    }


}

/*
 * check a given message that pertains to a player
 */
bool check_player_statement(struct Player* player, char* input, char* prefix,
        int prefixLength) {
    return strncmp(input, prefix, prefixLength) == 0 && 
            input[prefixLength] - 'A' < player->players;
}

/*
 * get input, and check to see if it matches the given string 
 * with strncmp
 *
 * otherwise quit to the appropriate error
 */
void get_input(char* input, char* comparison, int length) {
    fgets(input, 32, stdin);
    fprintf(stderr, "From StLucia:%s", input);
    fflush(stderr);

    if (strncmp(input, comparison, length)) {
        player_quit(5);
    } 

    if (feof(stdin)) {
        player_quit(4);
    }
}

/*
 * update's the players understanding of each player's health
 */
void update_health(struct Player* player, char playerLabel, char* input) {
    char* afterNumber = 0;
    int health = strtol(input, &afterNumber, 10);

    if (!strcmp(afterNumber, " in\n")) {

        player->othersHealth[player->hubPlayer - 'A'] -= health;

    } else if (!strcmp(afterNumber, " out\n")) {
        for (int i = 0; i < player->players; i++) {
            if (playerLabel != i + 'A') {
                player->othersHealth[i] -= health;
            }
        }

    } else {
        player_quit(5);
    }
    player->health = player->othersHealth[player->label - 'A'];
}

/*
 * check arguments and initialise player atributes 
 */
void player_init(struct Player* player, int playerArgCount,
        char** playerArgs) {

    if (playerArgCount != 3) {
        player_quit(1);
    }

    //check that number of players is in bounds
    int players;
    char* notAllDigits = 0;
    players = strtol(playerArgs[1], &notAllDigits, 10);
    if (players < 2 || players > 26 || *notAllDigits) {
        player_quit(2);
    }
    player->playersLeft = player->players = (uint8_t)players;

    //check that label is valid
    if (playerArgs[2][0] > 'Z' || playerArgs[2][0] < 'A' || playerArgs[2][1]) {
        player_quit(3);
    }
    player->label = playerArgs[2][0];
    player->health = 10;
    player->tokens = 0;
    player->score = 0;
    player->hubPlayer = players + 'A';

    player->othersHealth = malloc(sizeof(uint8_t) * players);
    memset(player->othersHealth, 10, sizeof(uint8_t) * players);

    putchar('!');
    fflush(stdout);
}

/*
 * take a given set of rolls and rerolls them accordingly
 * returns true if any rerolls occured
 */
bool parse_rolls(struct Player* player, char* rollMessage, int offset) {

    if (!valid_roll(rollMessage + offset, 6)) {
        player_quit(5);
    }

    char rerolls[14]; //'reroll ' + 6 dice
    memset(rerolls, 0, sizeof(char) * 13); 
    strcpy(rerolls, "reroll ");
    choose_rerolls(player, rerolls + 7, rollMessage + offset);

    if (rerolls[7] == 0 || rerolls[6] == 0) {
        return false;
    }

    printf("%s\n", rerolls);
    fflush(stdout);
    fflush(stdout);
    return true;
}


/*
 * initialises the signal handlers
 */
void init_signal_handlers(void) {
    struct sigaction signalAction;
    signalAction.sa_handler = player_quit;
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &signalAction, NULL);

    struct sigaction sigintAction;
    sigintAction.sa_handler = SIG_IGN;
    sigintAction.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sigintAction, NULL);
}

/*
 * handles the SIGPIPE call
 */
void sigpipe_handler(int signalNumber) {
    player_quit(4);
}

/*
 * prints an error message corresponding to the given value,
 * and exits using that value as an exit status
 */
void player_quit(int exitValue) {
    switch (exitValue) {
        case 1:
            fprintf(stderr, "Usage: player number_of_players my_id\n");
            exit(1);
        case 2:
            fprintf(stderr, "Invalid player count\n");
            exit(2);
        case 3:
            fprintf(stderr, "Invalid player ID\n");
            exit(3);
        case 4:
            fprintf(stderr, "Unexpectedly lost contact with StLucia\n");
            exit(4);
        case 5:
            fprintf(stderr, "Bad message from StLucia\n");
            exit(5);
    }
}

/*
 * updates the player's understanding of a healing player's health
 * based on the most recent roll
 */
void evaluate_health_rolls(struct Player* player,
        char toBeHealed, char* rolls) {
    int healer = toBeHealed - 'A';
    if (toBeHealed == player->hubPlayer) {
        return;
    }

    int toHeal = count_rolls(rolls, 6, 'H');
    if (player->othersHealth[healer] + toHeal > 10) {
        player->othersHealth[healer] = 10;
    } else {
        player->othersHealth[healer] += toHeal;
    }

    if (healer + 'A' == player->label) {
        player->health = player->othersHealth[healer];
    }
}
