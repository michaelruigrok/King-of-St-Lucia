#include "player.h"
#include "koslShared.h"

/*
 * fills rerolls with dice the player wishes to reroll
 */
void choose_rerolls(struct Player* player, char* rerolls, char* rolls) {
    for (int i = 0; i < 6; i++) {
        if (get_dice_val(rolls[i]) == -1) {
            player_quit(5);
        }

        if (player->hubPlayer == player->label) {
            if (rolls[i] == 'P') {
                continue;
            }

        } else {
            if (rolls[i] == 'A') {

                int occurrences = count_rolls(rolls + i, 6, 'A');
                int hubPlayer = player->hubPlayer - 'A';
                if (hubPlayer != player->players && 
                        occurrences >= player->othersHealth[hubPlayer]) {
                    i += occurrences;
                    continue;
                }
            }
        }
        *(rerolls++) = rolls[i];

    }
}

/*
 * return whether or not the player wishes to stay in St Lucia
 */
bool decide_if_staying(struct Player* player) {
    return true;
    
}

