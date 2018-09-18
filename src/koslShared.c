#include "koslShared.h"
#include <stdlib.h>

/*
 * return the value for a given dice face
 * or -1 if invalid face
 */
int get_dice_val(char face) {
    if ((face >= '1' && face <= '3')) {
        return face - '1' + 1;
    } else if (face == 'H') {
        return 4;
    } else if (face == 'A') {
        return 5;
    } else if (face == 'P') {
        return 6;
    } else {
        return -1;
    }
}

/*
 * return whether or not a collection of rolls was valid
 *
 * requires character after the last roll
 */
bool valid_roll(char* roll, int rollLength) {
    for (int face = 0; face < rollLength; face++) {
        if (get_dice_val(roll[face]) == -1) {
            return false;
        } 
    }
    return (roll[rollLength] == 0 || roll[rollLength] == '\n');


}


/*
 * return face1 - face2 of their respective values
 */
int dice_sort(const void* face1, const void* face2) {
    const char* face1Converted = (const char*)face1;
    const char* face2Converted = (const char*)face2;
    return get_dice_val(*face1Converted) - get_dice_val(*face2Converted);
}

/*
 * return the number of times a face has occurred in a roll
 */
int count_rolls(char* rolls, int rollsNumber, char face) {
    int occurrences = 0;
    for (int i = 0; i < rollsNumber; i++) {

        if ((rolls[i] == face)) {
            while (rolls[i + occurrences] == rolls[i] &&
                    i + occurrences < rollsNumber) {
                occurrences++;
            }
        }
    }
    return occurrences;
}

/*
 * if the given string starts with a valid number,
 * return the first non-number character as a string
 *
 * otherwise return null
 */
char* valid_number(char* numberString) {

    char nullValue = 0;
    char* afterNumber = 0;

    int number = strtol(numberString, &afterNumber, 10);

    return (number < 0 || number > 20) ? &nullValue : afterNumber;
}
