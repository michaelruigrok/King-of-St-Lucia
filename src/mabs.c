#include "player.h"
#include "koslShared.h"
#include <string.h>

/*
 * fills rerolls with dice the player wishes to reroll
 */
void choose_rerolls(struct Player* player, char* rerolls, char* rolls) {

    char save = (player->hubPlayer == player->label ? 'A' : 'H');

    for (int i = 0; i < 6; i++) {
        if (get_dice_val(rolls[i]) == -1) {
            player_quit(5);
        }

        if (rolls[i] == save || rolls[i] == '3') {
            continue;
        } else {
            *(rerolls++) = rolls[i];
        }
    }
}

/*
 * return whether or not the player wishes to stay in St Lucia
 */
bool decide_if_staying(struct Player* player) {
    return false;
}
