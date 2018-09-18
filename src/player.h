#include <stdbool.h>
#include <stdint.h>

struct Player {
    uint8_t players;
    uint8_t playersLeft;
    char label;
    int8_t health;
    uint8_t tokens;
    uint8_t score;

    int8_t* othersHealth;

    char hubPlayer;
};

/* Prototypes */
void parse_input(struct Player* player, char* input);
void get_input(char* input, char* comparison, int length);
void player_init(struct Player* player, int playerArgCount, char** playerArgs);
bool check_player_statement(struct Player* player, char* input, char* prefix,
        int prefix_length);
void init_signal_handlers(void);
void sigpipe_handler(int signalNumber);
void choose_rerolls(struct Player* player, char* rerolls, char* rolls);
bool parse_rolls(struct Player* player, char* roll_message, int offset);
bool decide_if_staying(struct Player* player);
void player_quit(int exitValue);
void update_health(struct Player* player, char playerLabel, char* input);
void evaluate_health_rolls(struct Player* player,
        char healer, char* rolls);
