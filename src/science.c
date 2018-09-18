#include "player.h"
#include "koslShared.h"

/*
 * fills rerolls with dice the player wishes to reroll
 */
void choose_rerolls(struct Player* player, char* rerolls, char* rolls) {
    char save = player->health < 5 ? 'H' : 'A';

    for (int i = 0; i < 6; i++) {
        if (get_dice_val(rolls[i]) == -1) {
            player_quit(5);
        }

        if (rolls[i] == save) {
            continue;
        } else {
            *(rerolls++) = rolls[i];
        }
    }
    *rerolls = 0;
}

/*
 * return whether or not the player wishes to stay in St Lucia
 */
bool decide_if_staying(struct Player* player) {
    return false;
}
