#include "hw1.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

extern char *polybius_alphabet;
extern const char *fm_alphabet;
extern const char *key;
extern const char *morse_table[];
extern const char *fm_alphabet;
extern const char *fractionated_table[];

extern char fm_key[27];

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the program
 * and will return a unsigned short (2 bytes) that will contain the
 * information necessary for the proper execution of the program.
 *
 * IF -p is given but no (-r) ROWS or (-c) COLUMNS are specified this function
 * MUST set the lower bits to the default value of 10. If one or the other
 * (rows/columns) is specified then you MUST keep that value rather than assigning the default.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return Refer to homework document for the return value of this function.
 */


int findLength(const char* string){ // Finds the length of a null terminated constant string
    int ret = 0;
    while(*(string + ret) != '\0'){
        ret++;
    }
    return ret;
}

int findArrLength(const char** arr){ // Finds the length of a null terminated constant string
    int ret = 0;
    while(*(arr + ret) != '\0'){
        ret++;
    }
    return ret;
}

int strCompare(char* one, char* two){ // Compares two character arrays. Returns 1 if equal and 0 if not equal.
    int i = 0;
    while(*(one + i) != '\0' || *(two + i) != '\0'){
        if(*(one + i) != *(two + i)){
            return 0; //False
        }
        ++i;
    }
    return 1; // True
}

int checkDup(char* potKey){ // Checks the potential key for duplicate characters. Returns 1 is none and 0 for detected duplicates.
    int strLen = findLength(potKey);
    if(strLen == 1){
        return 1;
    }
    for(int i = 1; i < strLen; i++){
        for(int j = 0; j < strLen; j++){
            if(i != j && *(potKey + i) == *(potKey + j)){
                return 0;
            }
        }
    }
    return 1;
}

int inString(char* potKey, const char* alphabet){ // Checks if all chars in the potential key are in the alphabet. 1 if they are, 0 otherwise.
    int keyLen = findLength(potKey);
    int alphaLen = findLength(alphabet);
    for(int i = 0; i < keyLen; i++){
        int charInStr = 0;
        for(int j = 0; j < alphaLen; j++){
            if(*(potKey + i) == *(alphabet + j)){
                charInStr = 1;
            }
        }
        if(charInStr == 0){
            return 0;
        }
    }
    return 1;
}

int verifyKey(char* potKey, int pFlag, int fFlag){
    int noDup = checkDup(potKey);
    if(noDup == 0){ // Duplicate detected
        return 0;
    }
    int inStr = 0;
    if(pFlag){
        inStr = inString(potKey, polybius_alphabet);
    }
    else if(fFlag){
        inStr = inString(potKey, fm_alphabet);
    }
    else{
        return 0;
    }
    key = potKey;
    return inStr; // 1 if potential key is in the string, 0 otherwise
}

int verifyPos(char* pos){
    int length = findLength(pos);
    if(length <= 0 || length > 2){ // Bounds
        return 0;
    }
    if(length == 1 && *pos == '9'){ // 9 is the lower limit for the rows and columns
        return 9;
    }
    else if(*pos == '1' && (*(pos + 1) >= '0' && *(pos + 1) < '6')){
        return 10 + *(pos + 1) - 48;
    }
    else {
        return 0;
    }
}

int opFlagVer(char **argv, int argNum, int pFlag, int fFlag, int *kFlagGiven, int *rFlagGiven, int *cFlagGiven, int *ret){
    int kFlag = strCompare("-k", *(argv + argNum));
    int rFlag = strCompare("-r", *(argv + argNum));
    int cFlag = strCompare("-c", *(argv + argNum));

    if(!(kFlag || rFlag || cFlag)){
        return 0;
    }

    if(kFlag){
        if(*kFlagGiven > 0){ // Duplicate flag check
            return 0;
        }
        *kFlagGiven += 1;
        int keyError = verifyKey(*(argv + argNum + 1), pFlag, fFlag);
        if(keyError == 0){
            return 0;
        }
        return 1;
    }
    else if(rFlag){
        if(*rFlagGiven > 0){ // Duplicate flag check
            return 0;
        }
        *rFlagGiven += 1;
    }
    else if(cFlag){
        if(*cFlagGiven > 0){ // Duplicate flag check
            return 0;
        }
        *cFlagGiven += 1;
    }
    else{
        return 0;
    }

   if(fFlag){ // Fractionated Morse does not accept row or column flags
        return 0;
    }
    int posError = verifyPos(*(argv + argNum + 1));
    if(posError == 0){
        return 0;
    }
    if(rFlag){
        *ret += (16 * (posError - 10));
    }
    else{
        *ret += (posError - 10);
    }
    return posError;
}

unsigned short validargs(int argc, char **argv) {
    if(argc <= 1){ // Check if any flags are given
        return 0;
    }

    int hFlag = strCompare("-h", *(argv + 1));
    int pFlag = strCompare("-p", *(argv + 1));
    int fFlag = strCompare("-f", *(argv + 1));
    if(hFlag){ // 1 = True; 0 = False
        return 0x8000;
    }
    if(argc <= 2){ // Invalid number of arguments for either ciphers
        return 0;
    }

    int eFlag = strCompare("-e", *(argv + 2));
    int dFlag = strCompare("-d", *(argv + 2));
    int ret = 0x0000; // Return variable

    if(eFlag || dFlag){
        if(dFlag){ // Changes the 13th bit. 1 if decrypt, 0 if encrypt.
            ret += 0x2000;
        }
    }
    else{
        return 0;
    }


    // Check for the Polybius/Fractionated Morse flags
    if(pFlag || fFlag){ // Checks whether the flags are given. Changes the 14th bit if they are.
                        // 1 if Fractionated Morse Cipher, 0 if Polybius Cipher.
        if(fFlag && argc <= 5){ //Fractionated Morse Cipher: ./hw1 -f -e|-d -k KEY
            ret += 0x4000;
        }
        else if (argc > 9){ // Polybius Cipher: ./hw1 -p -e|-d -k KEY -r ROW -c COLUMN
            return 0; // Argument count cnanot exceed 9 for the Polybius Cipher
        }
        else{
            ret += 0x00AA; // Row and columns default to 10.
        }
    }
    else{
        return 0;
    }


    // Check for the key flag and validate the given key
    int kFlagGiven = 0;
    int rFlagGiven = 0;
    int cFlagGiven = 0;
    key = "\0"; // Initialize the key value
    int rowColProduct = 1;

    int opFlagCounter = 3;
    while (opFlagCounter < argc){
        int opFlag = opFlagVer(argv, opFlagCounter, pFlag, fFlag, &kFlagGiven, &rFlagGiven, &cFlagGiven, &ret);
        if(opFlag == 0){
            return 0;
        }
        rowColProduct *= opFlag;
        opFlagCounter += 2;
    }

    if(fFlag){
        return ret;
    }

    int alpLen = findLength(polybius_alphabet);
    if(rFlagGiven && cFlagGiven && rowColProduct < alpLen){ // Check if row * col >= Polybius Cipher length
        return 0;
    }

    return ret;
}


//================================================ Part 2 -- Polybius Cipher ===============================================================

int letterInString(const char *string, char character){ // Returns 1 if a character is in a string, 0 otherwise.
    for(int i = 0; i < findLength(string); i++){
        if(character == *(string + i)){
            return 1; // Character is in the key
        }
    }
    return 0; // Character is not in the key;
}

void printPolyBoard(unsigned short row, unsigned short col){
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            int pos = i * col + j;
            printf("%c ", *(polybius_table + pos));
        }
        printf("\n");
    }
}

unsigned short row, col;

void polybiusTable(unsigned short mode){
    unsigned short rowKey = 0x00F0;
    unsigned short colKey = 0x000F;
    row = (mode & rowKey);
    row = (row >> 4);
    col = (mode & colKey);
    unsigned short boardSize = row * col;
    int alphaLen = findLength(polybius_alphabet);
    int keyLen = findLength(key);
    int i;
    char* polyPos;
    if(*key == '\0'){ // If there is no key
        for(i = 0; i < boardSize; i++){
            polyPos = polybius_table + i;
            if(i >= alphaLen){
                *polyPos = '\0';
            }
            else{
                char currChar = *(polybius_alphabet + i);
                *polyPos = currChar;
            }
        }
    }
    else{
        for(i = 0; i < keyLen; i++){
            polyPos = polybius_table + i;
            *polyPos = *(key + i);
        }
        int j = 0;
        for(i = i; i < boardSize; i++){
            polyPos = polybius_table + i;
            if(i >= alphaLen){
                *polyPos = '\0';
            }
            else{
                int inKey = letterInString(key, *(polybius_alphabet + j));
                while(inKey){
                    j++;
                    inKey = letterInString(key, *(polybius_alphabet + j));
                }
                *polyPos = *(polybius_alphabet + j);
                j++;
            }
        }
    }
}

int polybiusEncrypt(){
    char input = getchar();
    int alphaLen = findLength(polybius_alphabet);
    while(input != EOF){
        if(input == '\t'){ // Tab
            printf("\t");
        }
        else if(input == ' '){ // Space
            printf(" ");
        }
        else if(input == '\n'){ // New line
            printf("\n");
        }
        else if(letterInString(polybius_alphabet, input) == 0){
            return 0;
        }
        else{
            int charPos;
            for(int i = 0; i < alphaLen; i++){
                if(*(polybius_table + i) == input){
                    charPos = i;
                    break;
                }
            }
            int charRow = (int)(charPos/col);
            int charCol = charPos % col;
            if(charRow >= 10){
                printf("%c", charRow + 55);
            }
            else{
                printf("%d", charRow);
            }
            if(charCol >= 10){
                printf("%c", charCol + 55);
            }
            else{
                printf("%d", charCol);
            }
        }
        input = getchar();
    }
    return 1;
}

int polybiusDecrypt(){
    char inputRow = getchar(), inputCol;
    int inpRow, inpCol, pos;
    while(inputRow != EOF){
        if(inputRow == '\t'){ // Tab
            printf("\t");
        }
        else if(inputRow == ' '){ // Space
            printf(" ");
        }
        else if(inputRow == '\n'){ // New line
            printf("\n");
        }
        else{
            inputCol = getchar();
            if(inputRow >= 'A'){
                inpRow = (int)(inputRow) - 55;
            }
            else{
                inpRow = (int)(inputRow) - 48;
            }
            if(inputCol >= 'A'){
                inpCol = (int)(inputCol) - 55;
            }
            else{
                inpCol = (int)(inputCol) - 48;
            }
            pos = inpRow * col + inpCol;
            printf("%c", *(polybius_table + pos));
        }
        inputRow = getchar();
    }
    return 1;
}

//================================================ Part 3 -- Fractionated Morse Cipher ======================================================

void printFKey(){
    int fKeyLen = findLength(fm_key);
    for(int i = 0; i < fKeyLen; i++){
        printf("%c ", *(fm_key + i));
    }
    printf("\n");
}

int letterInStringC(const char *string, char character, int constraint){ // Returns 1 if a character is in a string, 0 otherwise.
    for(int i = 0; i < constraint; i++){
        if(character == *(string + i)){
            return 1; // Character is in the key
        }
    }
    return 0; // Character is not in the key;
}

void fmTable(){
    int alphaFLen = findLength(fm_alphabet);
    int fKeyLen = findLength(fm_key);
    int i;
    char* fPos;
    if(*key == '\0'){ // If there is no key
        for(i = 0; i < alphaFLen; i++){
            fPos = fm_key + i;
            char currChar = *(fm_alphabet + i);
            *fPos = currChar;
        }
    }
    else{
        for(i = 0; i < findLength(key); i++){
            fPos = fm_key + i;
            *fPos = *(key + i);
        }
        int j = 0;
        for(i = i; i < alphaFLen; i++){
            fPos = fm_key + i;
            int inKey = letterInString(key, *(fm_alphabet + j));
            while(inKey){
                j++;
                inKey = letterInString(key, *(fm_alphabet + j));
            }
            *fPos = *(fm_alphabet + j);
            j++;
        }
    }
}

int fMorseEncrypt(){
    char input = getchar();
    char *fmBuffer = polybius_table;
    int bufferEnd = 0;
    char *bufferPos = '\0';
    int fmTableLen = findArrLength(fractionated_table);
    while(input != EOF){
        if(input == '\t' || input == ' ' || input == '\n' || input == '\0'){ // Whitespace
            bufferPos = fmBuffer + bufferEnd;
            *bufferPos = 'x';
            bufferEnd++;
            if(input == '\n'){
                if(bufferEnd >= 3){
                    for(int i = 0; i < fmTableLen; i++){
                        if(*fmBuffer == **(fractionated_table + i)
                            && *(fmBuffer + 1) == *(*(fractionated_table + i) + 1)
                            && *(fmBuffer + 2) == *(*(fractionated_table + i) + 2)){
                            printf("%c", *(fm_key + i));
                            break;
                        }
                    }
                }
                printf("\n");
                bufferEnd = 0;
            }
        }
        else if (input < 0x21 || input > 0x7A){
            return 0;
        }
        else{
            const char *morseInp = *(morse_table + (input - 0x21));
            int morseInpLen = findLength(morseInp);
            if(morseInpLen == 0){
                return 0;
            }
            for(int i = 0; i < morseInpLen; i++){
                bufferPos = fmBuffer + bufferEnd;
                *bufferPos = *(morseInp + i);
                bufferEnd++;
            }
            bufferPos = fmBuffer + bufferEnd;
            *bufferPos = 'x';
            bufferEnd++;
            while(bufferEnd >= 3){
                for(int i = 0; i < fmTableLen; i++){
                    if(*fmBuffer == **(fractionated_table + i)
                        && *(fmBuffer + 1) == *(*(fractionated_table + i) + 1)
                        && *(fmBuffer + 2) == *(*(fractionated_table + i) + 2)){
                        printf("%c", *(fm_key + i));
                        break;
                    }
                }

                for(int i = 3; i < bufferEnd; i++){
                    char* newPos = fmBuffer + i - 3;
                    *newPos = *(fmBuffer + i);
                }
                bufferEnd -= 3;
            }
        }

        input = getchar();
    }
    return 1;
}

int fMorseDecrypt(){
    char input = getchar();
    char *fmBuffer = polybius_table;
    int bufferEnd = 0;
    char *bufferPos = '\0';
    int fmTableLen = findArrLength(fractionated_table); // Same length as fm_key
    int morseTableLen = findArrLength(morse_table);
    int morseInpLen;
    int word = 0;
    while(input != EOF){
        if(input != '\n' && input != '\0'){
            int inpPos;
            for(inpPos = 0; inpPos < fmTableLen; inpPos++){
                if(input == *(fm_key + inpPos)){
                    break;
                }
            }
            morseInpLen = findLength(*(fractionated_table + inpPos));
            for(int i = 0; i < morseInpLen; i++){
                bufferPos = fmBuffer + bufferEnd;
                *bufferPos = *(*(fractionated_table + inpPos) + i);
                bufferEnd++;
            }

            while(letterInStringC(fmBuffer, 'x', bufferEnd)){
                if(bufferEnd >= 2 && *(fmBuffer) == 'x' && *(fmBuffer + 1) == 'x'){
                    printf(" ");
                    for(int j = 2; j < bufferEnd; j++){
                        char* newPos = fmBuffer + j - 2;
                        *newPos = *(fmBuffer + j);
                    }
                    bufferEnd -= 2;
                }
                else if(bufferEnd >= 1 && *(fmBuffer) == 'x'){
                    for(int j = 1; j < bufferEnd; j++){
                        char* newPos = fmBuffer + j - 1;
                        *newPos = *(fmBuffer + j);
                    }
                    bufferEnd--;
                    if(word){
                        printf(" ");
                        word = 0;
                    }
                }
                else{
                    for(morseInpLen = 0; morseInpLen < bufferEnd; morseInpLen++){
                        if(*(fmBuffer + morseInpLen) == 'x'){
                            break;
                        }
                    }
                    for(int i = 0; i < morseTableLen; i++){
                        if(findLength(*(morse_table + i)) == morseInpLen){
                            int equal = 1;
                            for(int j = 0; j < morseInpLen; j++){
                                if(*(fmBuffer + j) != *(*(morse_table + i) + j)){
                                    equal = 0;
                                    break;
                                }
                            }
                            if(equal){
                                printf("%c", i + 0x21);
                                for(int j = morseInpLen; j < bufferEnd; j++){
                                    char* newPos = fmBuffer + j - morseInpLen;
                                    *newPos = *(fmBuffer + j);
                                }
                                bufferEnd -= morseInpLen;
                                if(bufferEnd >= 1 && *(fmBuffer) == 'x'){
                                    for(int j = 1; j < bufferEnd; j++){
                                        char* newPos = fmBuffer + j - 1;
                                        *newPos = *(fmBuffer + j);
                                    }
                                    word = 1;
                                }
                                bufferEnd--;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else{
            if(bufferEnd != 0){
                int i;
                for(i = 0; i < morseTableLen; i++){
                    if(findLength(*(morse_table + i)) == bufferEnd){
                        int equal = 1;
                        for(int j = 0; j < bufferEnd; j++){
                            if(*(fmBuffer + j) != *(*(morse_table + i) + j)){
                                equal = 0;
                                break;
                            }
                        }
                        if(equal){
                            printf("%c", i + 0x21);
                        }
                    }
                }
            }
            word = 0;
            bufferEnd = 0;
            *bufferPos = '\0';
            printf("\n");
        }
        input = getchar();
    }



    return 1;
}