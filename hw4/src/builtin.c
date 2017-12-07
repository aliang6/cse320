#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "csapp.h"

#include "sfish.h"
#include "debug.h"
#include <string.h>
#include <errno.h>

void builtin_help(){
    int child_status;
    pid_t pid;
    if((pid = fork()) == -1){
        printf(EXEC_ERROR, "Fork error");
    }
    else if(pid == 0){
        write(1, "Builtin Help\n", 13);
    }
    else{
        wait(&child_status);
    }

}

void builtin_exit(){
    //printf("Builtin Exit\n");
    exit(EXIT_SUCCESS);
}

char* builtin_cd(char* input, char* cur_dir, char* prev_dir){
    errno = 0;
    int ret = 0;
    //printf("Builtin cd\n");
    // (cd -) - Changes to last working directory
    if(memcmp("cd - ", input, 5) == 0){
        //printf("Builtin cd -\n");
        //printf("Prev dir is now %s\n", prev_dir);
        // If there is no previous path
        if(prev_dir == NULL){
            //printf("bash: cd: OLDPWD not set\n");
            printf(BUILTIN_ERROR, "bash: cd: OLDPWD not set");
            return NULL;
        }
        // Else redirect to previous path
        else{
            //printf("Prev dir is %s\n", prev_dir);
            ret = chdir(prev_dir);
        }
    }

    // (cd) - No argument changes to home directory (Stored in HOME env var)
    else if(strlen(input) == 3){ // Tokenize function creates a space after every token
        //printf("Builtin cd no argument\n");
        chdir(getenv("HOME"));
    }

    // (cd ./) - Reference to current working directory
    else if(memcmp("cd ./", input, 5) == 0){
        //printf("Builtin cd ./\n");
        char* path = (char*)calloc(2 * strlen(cur_dir) + 20, sizeof(char));
        if(strlen(input) > 5){
            char* token = strtok(input, " ");
            token = strtok(NULL, " "); // Second argument of input is the specified path
            memcpy(path, cur_dir, strlen(cur_dir));
            strcat(path, token + 1);
        }
        //printf("Path is %s\n", path);
        ret = chdir(path);
        free(path);
    }

    // (cd ../) - Reference to previous directory
    else if(memcmp("cd ../", input, 6) == 0){
        //printf("Builtin cd ../\n");
        char* path = (char*)calloc(2 * strlen(cur_dir) + 20, sizeof(char));
        int counter = strlen(cur_dir) - 1;
        //printf("Current directory is %s\n", cur_dir);
        if(strlen(cur_dir) > 1){
            if(cur_dir[counter] == '/'){
                counter--;
            }
            while(cur_dir[counter] != '/'){
                counter--;
            }
            //printf("Counter value is %d\n", counter);
            memcpy(path, cur_dir, counter);
            //printf("Path is %s\n", path);
            if(strlen(input) > 6){
                char* token = strtok(input, " ");
                token = strtok(NULL, " "); // Second argument of input is the specified path
                strcat(path, token + 2);
            }
            ret = chdir(path);
            //printf("Path is %s\n", path);
        }
        free(path);
    }

    // (cd PATH) - Change directory to path
    // Also catch error cases
    else{
        //printf("Builtin cd PATH\n");
        char* path = (char*)calloc(2 * strlen(cur_dir) + 20, sizeof(char));
        char* temp = (char*)calloc(2 * strlen(input) + 20, sizeof(char));
        memcpy(temp, input, strlen(input));
        char* token = strtok(temp, " ");
        token = strtok(NULL, " "); // Second argument of input is the specified path
        memcpy(path, cur_dir, strlen(cur_dir));
        if(token[0] == '/'){
            ret = chdir(token);
            /*else{
                ret = -1;
                errno = ENOENT;
            }*/
        }
        else{
            strcat(path, "/");
            strcat(path, token);
            printf("Path is %s\n", path);
            ret = chdir(path);
        }
        free(path);
        free(temp);
    }

    if(ret == -1){
        if(errno == ENOENT){
            write(1, "sfish: ", 7);
            write(1, input, strlen(input));
            write(1, "No such file or directory\n", 27);
        }
        return prev_dir;
    }

    if(prev_dir == NULL){
        prev_dir = (char*)calloc(2 * strlen(cur_dir) + 20, sizeof(char));
    }
    else{
        prev_dir = realloc(prev_dir, 2 * strlen(cur_dir) + 20);
    }
    //printf("Current dir is now %s\n", cur_dir);
    strcpy(prev_dir, cur_dir); // Update prev_dir
    //printf("Prev dir is now %s\n", prev_dir);

    int dir_len = 1024;
    while(getcwd(cur_dir, dir_len) == NULL){
        //printf("Getcwd equals null\n");
        dir_len += 1024;
        cur_dir = realloc(cur_dir, dir_len);
    }

    if(memcmp("cd - ", input, 5) == 0 && prev_dir != NULL){ // Print out cur_dir if cd -
        printf("%s\n", cur_dir);
    }
    return prev_dir;
}

void builtin_pwd(char* cur_dir){
    int child_status;
    pid_t pid;
    if((pid = fork()) == -1){
        printf(EXEC_ERROR, "Fork error");
    }
    else if(pid == 0){
        write(1, cur_dir, strlen(cur_dir));
        write(1, "\n", 1);
    }
    else{
        wait(&child_status);
    }

}

//=========================== Child Process Builtins =========================================
void builtin_jobs(int* jid, char* proc_name, int jid_max){
    //printf("Jobs function\n");
    int i = 0;
    for(; i < jid_max; i++){
        if(jid[i] != 0){
            int j = 0;
            while(j < i){
                //printf("j is currently %d\n", j);
                //printf("String is %s\n", proc_name + j);
                //printf("String length is %lu\n", strlen(proc_name + j));
                j += strlen(proc_name + j) + 1;
            }
            printf(JOBS_LIST_ITEM, i + 1, (proc_name + j));
        }
    }
}

void builtin_fg(char* tok_input, int* pids){
    if(memcmp("%", tok_input + 4, 1) != 0){
        printf(SYNTAX_ERROR, "Doesn't contain percent\n");
    }
    else{
        int jid;
        if(sscanf(tok_input + 7, "%d", &jid) != 1){
            printf(SYNTAX_ERROR, "Invalid argument");
            return;
        }
        kill(getpgid(pids[jid]), SIGCONT);
    }
}

void builtin_kill(char* tok_input, int* pids){
    if(memcmp("%", tok_input + 6, 1) == 0 ){
        //printf("JID kill\n");
        int jid;
        if(sscanf(tok_input + 7, "%d", &jid) != 1){
            printf(SYNTAX_ERROR, "Invalid argument");
            return;
        }
        kill(pids[jid], SIGKILL);
    }
    else{
        //printf("PID kill\n");
        pid_t pid;
        if(sscanf(tok_input + 6, "%d", &pid) != 1){
            printf(SYNTAX_ERROR, "Invalid argument");
            return;
        }
        kill(pid, SIGKILL);
    }
}
