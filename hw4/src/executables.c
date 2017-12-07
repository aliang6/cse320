#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "csapp.h"
#include "sfish.h"
#include "debug.h"


void exec_func(char* input){

    int child_status;
    pid_t pid;

    void child_handler(int sig){
        pid = wait(&child_status);
    }

    sigset_t mask, empty_set, full_set, prev_mask;

    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD); // Add SIGCHLD to mask set
    Sigemptyset(&empty_set);
    Sigfillset(&full_set);

    Signal(SIGCHLD, child_handler);

    char** input_array = calloc(4 * strlen(input) + 80, sizeof(char));
    char* token;
    int pos = 0;
    token = strtok(input, " ");
    while(token != NULL){
        input_array[pos] = token;
        pos++;
        token = strtok(NULL, " ");
    }
    input_array[pos] = NULL;

    printf("Command verified\n");
    Sigprocmask(SIG_BLOCK, &mask, &prev_mask); // Block SIGCHLD
    if((pid = fork()) == -1){
        printf(EXEC_ERROR, "Fork error");
    }
    else if(pid == 0){
        printf("Hello from child\n");
        int exec_ret = 0;
        exec_ret = execvp(input_array[0], input_array);
        if(exec_ret == -1){
            printf(EXEC_ERROR, "No such file or directory");
        }
        exit(EXIT_SUCCESS);
    }
    else{
        pid = 0;
        while(!pid){
            Sigsuspend(&empty_set);

            Sigprocmask(SIG_SETMASK, &prev_mask, NULL); // Restore previous mask
            printf("Hello from parent\n");
            //
            free(input_array);
            printf("Child has been terminated\n");
        }
    }
}
