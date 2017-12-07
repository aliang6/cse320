#include <stdlib.h>

#include "hw1.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    unsigned short mode;

    mode = validargs(argc, argv);

    debug("Mode: 0x%X", mode);

    if(mode & 0x8000) {
        USAGE(*argv, EXIT_SUCCESS);
    }
    else if(mode == 0) {
        USAGE(*argv, EXIT_FAILURE);
    }

    /*char* input = "\0";
    int i = 0;
    char currChar;
    while(getchar() != '\0'){
        currChar = getchar();
        char* currPos = input + i;
        *currPos = currChar;
        i++;
    }*/

    if(mode & 0x4000){ // Fractionated Morse Cipher
        fmTable(); // Constructs the Fractionated Morse Key
        if(mode & 0x2000){
            mode = fMorseDecrypt();
            if(mode == 0) {
                if(mode == 0) {
                    USAGE(*argv, EXIT_FAILURE);
                }
            }
        }
        else{
            mode = fMorseEncrypt();
            if(mode == 0) {
                if(mode == 0) {
                    USAGE(*argv, EXIT_FAILURE);
                }
            }
        }
    }
    else{ // Polybius Cipher
        polybiusTable(mode); //Constructs the Polybius Table
        if(mode & 0x2000){
            mode = polybiusDecrypt();
            if(mode == 0) {
                USAGE(*argv, EXIT_FAILURE);
            }
        }
        else{
            mode = polybiusEncrypt();
            if(mode == 0) {
                USAGE(*argv, EXIT_FAILURE);
            }
        }
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */