#include <stdbool.h>
#include <stdint.h>

//information about the game's players
struct Players {
    uint8_t playerCount;
    int* processIds;
    int** descriptors;

    bool* eliminated;
    int8_t* health;
    uint8_t* tokens;
    unsigned int* score;
};

//information about the game's state
struct Game {
    uint8_t playerCount;
    char* rolls; //string containing all rolls
    int rollCount; //number of rolls in rolls
    int nextRoll; // the position of the next roll
    int pointsToWin; //no. points needed to win

    uint8_t inStLucia; //the player currently in St Lucia (else playerCount)
};

/* Prototypes */
void hub_quit(int exitValue);
void load_rollfile(struct Game* game, char* filename);
void init_players(struct Players* players, uint8_t playerCount,
        char** playerList);
void check_player_init(struct Players* players, int player);
void init_player_properties(struct Players* players);
char* get_response(struct Players* players, int player, char* response,
        int responseLength);
void init_signal_handlers(void);
void disable_signal_handlers(void);
void shutdown(int handler);
void game_loop(struct Game* game, struct Players* players);
char* manage_rerolls(struct Game* game, struct Players* players, int player, 
        char* rolls);
char* get_rolls(struct Game* game, char* rolls, int rollsNumber);
int evaluate_score_rolls(struct Game* game, struct Players* players,
        int player, char* rolls);
void evaluate_health_rolls(struct Game* game, struct Players* players,
        int player, char* rolls);
void evaluate_attack_rolls(struct Game* game, struct Players* players,
        int player, char* rolls);
bool damage_player(struct Players* players, int player, int damage);
bool ask_if_leaving(struct Game* game, struct Players* players, int player);
void win_check(struct Game* game, struct Players* players);
void winner(struct Players* players, int winner);
void kill_children(struct Players* players);
void kill_child(struct Players* players, int player);
