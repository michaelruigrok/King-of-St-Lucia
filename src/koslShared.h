#ifndef stdbool
#include <stdbool.h> 
#endif

/* prototypes */
int get_dice_val(char face);
int dice_sort(const void* face1, const void* face2);
int count_rolls(char* rolls, int rollsNumber, char face);
char* valid_number(char* numberString);
bool valid_roll(char* roll, int roll_length);
