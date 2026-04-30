/*
 * driver_display_characters.h
 *
 *  Created on: Apr 30, 2026
 *      Author: vanya
 */

#ifndef INC_DRIVER_DISPLAY_CHARACTERS_H_
#define INC_DRIVER_DISPLAY_CHARACTERS_H_
#define mk(hi, lo) (0b ## hi ## lo)


#define DISPLAY_CHAR_JA_DOT 		mk(1010, 0001)
#define DISPLAY_CHAR_JA_BRACK_LHS 	mk(1010, 0010)
#define DISPLAY_CHAR_JA_BRACK_RHS 	mk(1010, 0011)
#define DISPLAY_CHAR_JA_COMMA 		mk(1010, 0100)
#define DISPLAY_CHAR_JA_OR 			mk(1010, 0101)
#define DISPLAY_CHAR_JA_WO			mk(1010, 0110)
#define DISPLAY_CHAR_JA_SMALL_A 	mk(1010, 0111)
#define DISPLAY_CHAR_JA_SMALL_I 	mk(1010, 1000)
#define DISPLAY_CHAR_JA_SMALL_U		mk(1010, 1001)
#define DISPLAY_CHAR_JA_SMALL_E 	mk(1010, 1010)
#define DISPLAY_CHAR_JA_SMALL_O		mk(1010, 1011)
#define DISPLAY_CHAR_JA_SMALL_YA	mk(1010, 1100)
#define DISPLAY_CHAR_JA_SMALL_YU	mk(1010, 1101)
#define DISPLAY_CHAR_JA_SMALL_YO	mk(1010, 1110)
#define DISPLAY_CHAR_JA_SMALL_TSU	mk(1010, 1111)



#undef mk
#endif /* INC_DRIVER_DISPLAY_CHARACTERS_H_ */
