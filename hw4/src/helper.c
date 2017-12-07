#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include "csapp.h"

#include "sfish.h"
#include "debug.h"
#include <string.h>

char* tokenize_input(char* input){
    char* tokenized = calloc(4 * strlen(input) + 80, sizeof(char));
    tokenized[0] = '\0';
    char* token;
    int pos = 0;
    token = strtok(input, " \t\r\n\v\f");
    while(token != NULL){
        //printf("Token is %s\n", token);
        strcat(tokenized, token);
        pos += strlen(token);
        *(tokenized + pos) = ' ';
        //printf("Tokenized is %s\n", tokenized);
        pos++;
        token = strtok(NULL, " \t\r\n\v\f");
        *(tokenized + pos) = '\0';
    }
    *(tokenized + pos) = '\0';
    return tokenized;
}

void close_pipes(int* fd_array, int size){
    int i = 0;
    for(; i < size; i++){
        close(fd_array[i]);
    }
}