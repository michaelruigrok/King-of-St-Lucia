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

        if ((rolls[i] <= '3')) {
            int occurrences = count_rolls(rolls + i, 6 - i, rolls[i]);
            if (occurrences > 2) {
                i += occurrences - 1; // - 1 to offset regular loop iteration
                continue;
            }
        }

        if (player->health < 6 && rolls[i] == 'H') {
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
    return (player->health >= 5);
}
