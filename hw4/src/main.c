#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <readline/readline.h>
#include "csapp.h"

#include "sfish.h"
#include "debug.h"
#include <string.h>
#include <fcntl.h>


int main(int argc, char *argv[], char* envp[]) {
    int org_stdin = dup(STDIN_FILENO);
    int org_stdout = dup(STDOUT_FILENO);
    char* input = NULL;
    bool exited = false;
    char* prev_dir = NULL;
    char* cur_dir = NULL;
    char *tok_input = NULL;
    char** tokenized = NULL;
    char* prompt_color = KNRM;
    bool no_pipes = false;
    int* pids = calloc(100, sizeof(int));
    int* jid = calloc(100, sizeof(int));
    int jid_cur = 0;
    int jid_max = 100;
    char* proc_name = calloc(400, sizeof(char));
    proc_name[0] = '\0';
    int proc_cur = 0;
    int proc_max = 400;
    //int gl_child_status;
    pid_t process_group = -1;
    //volatile int spid = getppid();


    void exec_func1(char* input, int* jid, char* proc_name){ // Exec function
        //printf("Exec function\n");
        int child_status;
        pid_t pid;

        void child_handler(int sig){
            //printf("Child handler\n");
            //pid = Waitpid(pid, &child_status, WUNTRACED | WNOHANG | WCONTINUED);
            pid = wait(&child_status);
        }

        sigset_t mask, empty_set, full_set, prev_mask;

        Sigemptyset(&mask);
        Sigaddset(&mask, SIGCHLD); // Add SIGCHLD to mask set
        Sigaddset(&mask, SIGTTOU);
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

        if(jid_cur + 1 >= jid_max){
            //printf("Reallocing jid\n");
            jid = realloc(jid, jid_max + 100);
            pids = realloc(pids, jid_max + 100);
            jid_max += 100;
        }
        if(proc_cur + strlen(input_array[0]) + 1 >= proc_max){
            //printf("Reallocing proc_name\n");
            proc_name = realloc(proc_name, proc_max + 400);
            //printf("Reallocing proc_name\n");
            proc_max += 400;
        }

        //printf("Jid cur is %d, proc_cur is %d\n", jid_cur, proc_cur);
        jid[jid_cur] = jid_cur + 1;
        jid_cur++;
        if(proc_name[0] != '\0'){
            proc_name[proc_cur + 1] = '\0';
            strcpy(proc_name + proc_cur, input_array[0]);
        }
        else{
            strcat(proc_name, input_array[0]);
        }

        //printf("The copied process name is %s\n", (proc_name + proc_cur));
        proc_cur += (strlen(input_array[0]) + 1);
        //printf("Jid cur is %d, proc_cur is %d\n", jid_cur, proc_cur);

        Sigprocmask(SIG_BLOCK, &mask, &prev_mask); // Block SIGCHLD
        if((pid = fork()) == -1){
            printf(EXEC_ERROR, "Fork error");
        }
        else if(pid == 0){
            //printf("Hello from child\n");
            Signal(SIGINT, SIG_DFL);
            Signal(SIGTSTP, SIG_DFL);
            Sigprocmask(SIG_SETMASK, &empty_set, NULL); // Unblock SIGCHLD
            //printf("Hello again from child\n");
            int exec_ret = 0;
            exec_ret = execvp(input_array[0], input_array);
            if(exec_ret == -1){
                printf(EXEC_ERROR, "No such file or directory");
            }
            exit(EXIT_SUCCESS);
        }
        else{
            //printf("Hello from parent\n");
            if(process_group == -1){
                //printf("Setting up process group\n");
                process_group = getsid(pid);
            }
            else{
                //printf("Adding to process group\n");
                setpgid(pid, process_group);
            }
            //printf("Setting process group to foreground\n");
            //tcsetpgrp(STDIN_FILENO, process_group);
            //printf("Setting process group to foreground\n");
            //tcsetpgrp(STDIN_FILENO, spid);
            //printf("Setting process group to foreground\n");
            pid = 0;
            while(!pid){
                //printf("Hello from parent\n");
                pids[jid_cur] = pid;
                Sigsuspend(&empty_set);
                Sigprocmask(SIG_SETMASK, &prev_mask, NULL); // Restore previous mask
                //printf("Hello again from parent\n");
                jid[jid_cur - 1] = 0;
                //free(input_array);
                //printf("Child has been terminated\n");
            }
        }
    }

    void int_handler(int sig){
        //printf("SIGINT\n");
    }

    void stop_handler(int sig){
        //printf("SIGTSTP\n");
    }

    Signal(SIGINT, int_handler);

    Signal(SIGTSTP, stop_handler);

    //Signal(SIGCHLD, chld_handler);


    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }

    do {
        if(input != NULL){
            // Readline mallocs the space for input. You must free it.
            //printf("Freeing input\n");
            rl_free(input);
        }
        if(no_pipes && tok_input != NULL){
            // Free input which was set equal to a malloc in tokenize_input
            //printf("Freeing tok_input\n");
            free(tok_input);
            no_pipes = false;
        }
        if(tokenized != NULL){
            //printf("Freeing tokenized\n");
            free(tokenized);
        }

        //printf("Prev dir is now %s\n", prev_dir);
        if(cur_dir == NULL){
            int dir_len = 1024;
            cur_dir = (char*)calloc(dir_len, sizeof(char));
            while(getcwd(cur_dir, dir_len) == NULL){
                //printf("Getcwd equals null\n");
                dir_len += 1024;
                cur_dir = realloc(cur_dir, dir_len);
            }
        }
        char* user = " :: anlliang >> ";
        char* prompt = calloc(strlen(cur_dir) + strlen(user) + 4, sizeof(char));
        //printf("Current directory is %s\n", cur_dir);
        if(strlen(cur_dir) >= 12 && memcmp(getenv("HOME"), cur_dir, strlen(getenv("HOME"))) == 0){
            //printf("Change home directory\n");
            //prompt = (char*)malloc(strlen(cur_dir) - 13 + strlen(user) + 1);
            //prompt = prompt_color;
            prompt[0] = '~'; // Set first character to be ~
            prompt[1] = '\0';
            if(strlen(cur_dir) > 12){
                strcat(prompt + 1, cur_dir + 13);
            }
            //strcat(prompt + 1, home_dir);

        }
        else{
            //prompt = malloc(strlen(cur_dir) + strlen(user) + 1);
            strcpy(prompt, cur_dir);
        }
        //printf("Prompt is %s\n", prompt);
        strcat(prompt, user);
        printf("%s%s" KNRM, prompt_color, prompt);
        input = readline("");
        //printf("Input is %s\n", input);
        if(input == NULL){
            exit(EXIT_SUCCESS);
        }
        free(prompt);

        // Converts the input in a pointer array delimited by pipes (|)
        tokenized = calloc(4 * strlen(input) + 80, sizeof(char));
        char* token;
        int pos = 0;
        token = strtok(input, "|");
        bool mult_pipes = false;
        do {
            //printf("Token is %s\n", token);
            if((pos == 0 && token == NULL)){// || *token == '|'){
                mult_pipes = true;
                break;
            }
            tokenized[pos] = token;
            pos++;
            token = strtok(NULL, "|");
        } while(token != NULL);
        tokenized[pos] = NULL;
        //printf("Pos is %d\n", pos);

        if(mult_pipes){
            dprintf(org_stdout, SYNTAX_ERROR, "Syntax error");
            continue;
        }

        if(tokenized[0] == NULL || *(tokenized[0]) == ' ') {
            //printf("Input is null\n");
            continue;
        }


        /*write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, "SomeText", strlen("SomeText"));
        write(1, "\e[u", strlen("\e[u"));*/

        // If EOF is read (aka ^D) readline returns NULL

        if(pos == 1){ // No pipes
            no_pipes = true;
            if(memchr(tokenized[0], '<', strlen(tokenized[0])) != NULL && memchr(tokenized[0], '>', strlen(tokenized[0])) != NULL){
                // Contains right and left angles
                int right_angle_pos = -1, left_angle_pos = -1;
                int j = 0;
                //printf("Starting while loop\n");
                while(right_angle_pos == -1 || left_angle_pos == -1){
                    if(tokenized[0][j] == '<'){
                        left_angle_pos = j;
                        //printf("Left angle position is %d\n", left_angle_pos);
                    }
                    else if(tokenized[0][j] == '>'){
                        right_angle_pos = j;
                        //printf("Right angle position is %d\n", right_angle_pos);
                    }
                    j++;
                }
                //printf("Ended while loop\n");
                char** angles = calloc(4 * strlen(tokenized[0]) + 80, sizeof(char));
                char* token;
                int angles_pos = 0;
                token = strtok(input, "<>");
                while(token != NULL){
                    angles[angles_pos] = token;
                    angles_pos++;
                    //printf("Angle position is %d\n", angles_pos);
                    token = strtok(NULL, "<>");
                }
                if(angles_pos > 3){
                    dprintf(org_stdout, SYNTAX_ERROR, "Too many angles");
                    free(angles);
                    break;
                }

                // Remove white spaces from text file
                token = strtok(angles[1], " \t\r\n\v\f");
                angles[1] = token;
                token = strtok(angles[2], " \t\r\n\v\f");
                angles[2] = token;


                if(right_angle_pos > left_angle_pos){// one < two > three
                    //printf("angles[0] is %s, angles[1] is %s, and angles[2] is %s\n", angles[0], angles[1], angles[2]);
                    //printf("Starting left to right angle redirection\n");
                    pid_t pid;
                    int child_status;
                    int fd1 = open(angles[1], O_RDONLY, 0);
                    int fd2 = open(angles[2], O_WRONLY, 0);
                    if(fd1 == -1 || fd2 == -1){
                        dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                    }
                    else{
                        if((pid = fork()) == -1){
                            dprintf(org_stdout, EXEC_ERROR, "Fork error");
                        }
                        else if(pid == 0){
                            dup2(fd1, STDIN_FILENO);
                            dup2(fd2, STDOUT_FILENO);
                            exec_func1(angles[0], jid, proc_name);
                            exit(EXIT_SUCCESS);
                        }
                        else{
                            wait(&child_status);
                            dup2(org_stdout, STDOUT_FILENO);
                            dup2(org_stdin, STDIN_FILENO);
                        }
                    }

                    //printf("Completed left to right angle redirection\n");
                }
                else{ // one > two < three
                    //printf("angles[0] is %s, angles[1] is %s, and angles[2] is %s\n", angles[0], angles[1], angles[2]);
                    //printf("Starting right to left angle redirection\n");
                    pid_t pid;
                    int child_status;
                    int fd1 = open(angles[1], O_WRONLY, 0);
                    int fd2 = open(angles[2], O_RDONLY, 0);
                    if(fd1 == -1 || fd2 == -1){
                        dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                    }
                    else{
                        if((pid = fork()) == -1){
                            dprintf(org_stdout, EXEC_ERROR, "Fork error");
                        }
                        else if(pid == 0){
                            dup2(fd1, STDOUT_FILENO);
                            dup2(fd2, STDIN_FILENO);
                            exec_func1(angles[0], jid, proc_name);
                            exit(EXIT_SUCCESS);
                        }
                        else{
                            wait(&child_status);
                            dup2(org_stdout, STDOUT_FILENO);
                            dup2(org_stdin, STDIN_FILENO);
                        }
                    }

                    //printf("Completed right to left angle redirection\n");
                }

                free(angles);
            }

            else if(memchr(tokenized[0], '<', strlen(tokenized[0])) != NULL){ // Left angle
                //printf("Left angle detected\n");
                char** angles = calloc(4 * strlen(tokenized[0]) + 80, sizeof(char));
                char* token;
                int angles_pos = 0;
                token = strtok(input, "<");
                while(token != NULL){
                    angles[angles_pos] = token;
                    angles_pos++;
                    //printf("Angle position is %d\n", angles_pos);
                    token = strtok(NULL, "<");
                }
                if(angles_pos > 2){
                    dprintf(org_stdout, SYNTAX_ERROR, "Too many angles");
                    free(angles);
                    break;
                }

                // Remove white spaces from text file
                token = strtok(angles[1], " \t\r\n\v\f");
                angles[1] = token;

                //printf("angles[0] is %s and angles[1] is %s\n", angles[0], angles[1]);

                //printf("Starting left angle redirection\n");
                pid_t pid;
                int child_status;
                int fd1 = open(angles[1], O_RDONLY, 0);

                if(fd1 == -1){
                    dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                }
                else{
                    if((pid = fork()) == -1){
                        //printf(EXEC_ERROR, "Fork error");
                    }
                    else if(pid == 0){
                        dup2(fd1, STDIN_FILENO);
                        exec_func1(angles[0], jid, proc_name);
                        exit(EXIT_SUCCESS);
                    }
                    else{
                        wait(&child_status);
                        dup2(org_stdout, STDOUT_FILENO);
                        dup2(org_stdin, STDIN_FILENO);
                    }
                }

                free(angles);
                //printf("Completed left angle redirection\n");
            }

            else if(memchr(tokenized[0], '>', strlen(tokenized[0])) != NULL){ // Right angle
                //printf("Right angle detected\n");
                char** angles = calloc(4 * strlen(tokenized[0]) + 80, sizeof(char));
                char* token;
                int angles_pos = 0;
                token = strtok(input, ">");
                while(token != NULL){
                    angles[angles_pos] = token;
                    angles_pos++;
                    //printf("Angle position is %d\n", angles_pos);
                    token = strtok(NULL, ">");
                }
                if(angles_pos > 2 || angles_pos <= 1){
                    dprintf(org_stdout, SYNTAX_ERROR, "Too many angles");
                    free(angles);
                    continue;
                }

                // Remove white spaces from text file
                token = strtok(angles[1], " \t\r\n\v\f");
                angles[1] = token;

                //printf("angles[0] is %s and angles[1] is %s\n", angles[0], angles[1]);

                //printf("Starting right angle redirection\n");
                pid_t pid;
                int child_status;
                int fd1 = open(angles[1], O_WRONLY, 0);
                if(fd1 < 0){
                    dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                }
                else{
                    if((pid = fork()) == -1){
                        dprintf(org_stdout, EXEC_ERROR, "Fork error");
                    }
                    else if(pid == 0){
                        dup2(fd1, STDOUT_FILENO);
                        exec_func1(angles[0], jid, proc_name);
                        exit(EXIT_SUCCESS);
                    }
                    else{
                        wait(&child_status);
                        dup2(org_stdout, STDOUT_FILENO);
                        dup2(org_stdin, STDIN_FILENO);
                    }
                }

                free(angles);
                //printf("Completed right angle redirection\n");
            }
            else{
                //printf("Input is %s before tokenizing\n", tokenized[0]);
                tok_input = tokenize_input(tokenized[0]);
                //printf("Input is %s after tokenizing\n", tok_input);

                // Builtin help
                if(memcmp("help ", tok_input, 5) == 0){
                    builtin_help();
                    continue;
                }

                // Builtin exit
                else if(memcmp("exit ", tok_input, 5) == 0){
                    rl_free(input);
                    free(cur_dir);
                    free(tok_input);
                    free(prev_dir);
                    builtin_exit();
                }

                // Builtin change directory (cd)
                else if(memcmp("cd ", tok_input, 3) == 0){
                    prev_dir = builtin_cd(tok_input, cur_dir, prev_dir);
                    continue;
                    //printf("Prev dir after builtin is now %s\n", prev_dir);
                }

                // Builtin pwd
                else if(memcmp("pwd ", tok_input, 4) == 0){
                    builtin_pwd(cur_dir);
                    continue;
                }

                // Builtin color
                else if(memcmp("color ", tok_input, 6) == 0){
                    if(memcmp("RED", tok_input + 6, 3) == 0){
                        prompt_color = KRED;
                    }
                    else if(memcmp("GRN", tok_input + 6, 3) == 0){
                        prompt_color = KGRN;
                    }
                    else if(memcmp("YEL", tok_input + 6, 3) == 0){
                        prompt_color = KYEL;
                    }
                    else if(memcmp("BLU", tok_input + 6, 3) == 0){
                        prompt_color = KBLU;
                    }
                    else if(memcmp("MAG", tok_input + 6, 3) == 0){
                        prompt_color = KMAG;
                    }
                    else if(memcmp("CYN", tok_input + 6, 3) == 0){
                        prompt_color = KCYN;
                    }
                    else if(memcmp("WHT", tok_input + 6, 3) == 0){
                        prompt_color = KWHT;
                    }
                    else if(memcmp("BWN", tok_input + 6, 3) == 0){
                        prompt_color = KBWN;
                    }
                    else{
                        printf(SYNTAX_ERROR, "Invalid color");
                    }
                }

                else if(memcmp("jobs ", tok_input, 5) == 0){
                    //printf("Builtin jobs\n");
                    //printf("Jid_max is %d\n", jid_max);
                    builtin_jobs(jid, proc_name, jid_max);
                    continue;
                }

                else if(memcmp("kill ", tok_input, 5) == 0){
                    builtin_kill(tok_input, pids);
                }

                else if(memcmp("fg ", tok_input, 3) == 0){
                    builtin_fg(tok_input, pids);
                }

                else{
                    //printf("Executable\n");
                    //exec_func(tok_input);
                    exec_func1(tok_input, jid, proc_name);
                    continue;
                }
            }
            continue;
        }

        //printf("Tokenized[pos-1] is %s\n", tokenized[pos-1]);

        if(tokenized[pos - 1] == NULL || tokenized[pos - 1][strlen(tokenized[pos - 1]) - 1] == '|'){
            dprintf(org_stdout, SYNTAX_ERROR, "No argument after pipe");
            continue;
        }
        //int fd_array[2];
        //pipe(fd_array);

        // Create the pipe array
        int fd_array[(pos - 1) * 2];
        int fd_array_counter = 0;
        //printf("Creating fd_array\n");
        for(; fd_array_counter < pos; fd_array_counter++){
            //printf("The counter is at %d\n", fd_array_counter);
            if(pipe(fd_array + 2 * fd_array_counter) < 0){
                dprintf(org_stdout, EXEC_ERROR, "Pipe error\n");
                break;
            }
        }

        int i = 0;
        for(; i < pos; i++){
            if(i == 0){ // First pipe
                //printf("First pipe\n");
                if(memchr(tokenized[0], '>', strlen(tokenized[0])) != NULL){
                    dprintf(org_stdout, SYNTAX_ERROR, "Invalid command");
                    break;
                }
                else if(memchr(tokenized[0], '<', strlen(tokenized[0])) != NULL){
                    //printf("First pipe contains left angle\n");
                    char** angles = calloc(4 * strlen(tokenized[0]) + 80, sizeof(char));
                    char* token;
                    int angles_pos = 0;
                    token = strtok(tokenized[0], "<");
                    while(token != NULL){
                        angles[angles_pos] = token;
                        angles_pos++;
                        //printf("Angle position is %d\n", angles_pos);
                        token = strtok(NULL, "<");
                    }
                    if(angles_pos > 2){
                        dprintf(org_stdout, SYNTAX_ERROR, "Too many angles");
                        free(angles);
                        continue;
                    }

                    // Remove white spaces from text file
                    token = strtok(angles[1], " \t\r\n\v\f");
                    angles[1] = token;

                    //printf("angles[0] is %s and angles[1] is %s\n", angles[0], angles[1]);

                    //printf("Starting left angle redirection\n");
                    pid_t pid;
                    int child_status;
                    int fd1 = open(angles[1], O_RDONLY, 0);

                    if(fd1 == -1){
                        dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                    }
                    else{
                        if((pid = fork()) == -1){
                            dprintf(org_stdout, EXEC_ERROR, "Fork error");
                            break;
                        }
                        else if(pid == 0){
                            dup2(fd1, STDIN_FILENO);
                            dup2(fd_array[1], STDOUT_FILENO);
                            close(fd_array[0]);
                            exec_func1(angles[0], jid, proc_name);
                            exit(EXIT_SUCCESS);
                        }
                        else{
                            dup2(fd_array[0], STDIN_FILENO);
                            close(fd_array[1]);
                            wait(&child_status);
                            /*if(pos == 2){
                                //dup2(fd_array[0], STDIN_FILENO);
                                exec_func(tokenized[1]);
                                dup2(org_stdin, STDIN_FILENO);
                                dup2(org_stdout, STDOUT_FILENO);
                                printf("Completed single pipe redirection with left angle\n");
                                free(angles);
                                close_pipes(fd_array, pos);
                                break;
                            }*/
                            /*\\else{
                                dup2(fd_array[3], STDOUT_FILENO);
                                close(fd_array[0]);
                                exec_func(tokenized[1]);
                                printf("Completed first pipe redirection with left angle\n");
                            }*/
                        }
                    }

                    free(angles);
                    //printf("Completed left angle redirection\n");
                }
                else{
                    //printf("Regular piping\n");

                    //printf("Starting first pipe redirection\n");
                    pid_t pid;
                    int child_status;

                    if((pid = fork()) == -1){
                        dprintf(org_stdout, EXEC_ERROR, "Fork error");
                        close_pipes(fd_array, pos);
                        break;
                    }
                    else if(pid == 0){
                        dup2(fd_array[1], STDOUT_FILENO);
                        close(fd_array[0]);
                        exec_func1(tokenized[0], jid, proc_name);
                        dprintf(org_stdout, "In child\n");
                        exit(EXIT_SUCCESS);

                    }
                    else{
                        dup2(fd_array[0], STDIN_FILENO);
                        close(fd_array[1]);
                        wait(&child_status);
                        dprintf(org_stdout, "In parent\n");
                        if(pos == 2){
                            dprintf(org_stdout, "Pos is equal to 2\n");
                            dup2(org_stdout, STDOUT_FILENO);
                            dprintf(org_stdout, "Tokenized[1] is %s\n", tokenized[1]);
                            exec_func1(tokenized[1], jid, proc_name);
                            dprintf(org_stdout, "Completed single pipe redirection\n");
                            //close_pipes(fd_array, 2);
                            dup2(org_stdin, STDIN_FILENO);
                            break;
                        }
                        else{
                            //dup2(fd_array[1], STDOUT_FILENO);
                            //exec_func(tokenized[1]);
                            //printf("Completed first pipe redirection\n");
                        }
                    }

                    //printf("Completed first pipe redirection\n");
                }
            }
            else if(i == pos - 1){ // Last pipe
                //printf("Last pipe\n");
                if(memchr(tokenized[pos - 1], '<', strlen(tokenized[pos - 1])) != NULL){
                    dprintf(org_stdout, SYNTAX_ERROR, "Invalid command");
                    close_pipes(fd_array, pos);
                    break;
                }
                else if( // Right angle in last pipe
                    memchr(tokenized[pos - 1], '>', strlen(tokenized[pos - 1])) != NULL){
                    //printf("Last pipe contains right angle\n");
                    //printf("Right angle detected\n");
                    char** angles = calloc(4 * strlen(tokenized[pos - 1]) + 80, sizeof(char));
                    char* token;
                    int angles_pos = 0;
                    token = strtok(tokenized[pos - 1], ">");
                    while(token != NULL){
                        angles[angles_pos] = token;
                        angles_pos++;
                        //printf("Angle position is %d\n", angles_pos);
                        token = strtok(NULL, ">");
                    }
                    if(angles_pos > 2 || angles_pos <= 1){
                        dprintf(org_stdout, SYNTAX_ERROR, "Too many angles");
                        free(angles);
                        close_pipes(fd_array, pos);
                        break;
                    }

                    // Remove white spaces from text file
                    token = strtok(angles[1], " \t\r\n\v\f");
                    angles[1] = token;

                    //printf("angles[0] is %s and angles[1] is %s\n", angles[0], angles[1]);

                    //printf("Starting right angle redirection\n");
                    pid_t pid;
                    int child_status;
                    int fd1 = open(angles[1], O_WRONLY, 0);
                    dprintf(org_stdout, "FD is %d\n", fd1);
                    //dup2(fd_array[2*i - 2], STDIN_FILENO);
                    if(fd1 < 0){
                        dprintf(org_stdout, SYNTAX_ERROR, "Invalid file name");
                    }
                    else{
                        if((pid = fork()) == -1){
                            dprintf(org_stdout, EXEC_ERROR, "Fork error");
                        }
                        else if(pid == 0){
                            dup2(fd_array[2*i - 2], STDIN_FILENO);
                            dup2(fd1, STDOUT_FILENO);
                            close(fd_array[2*i - 1]);
                            exec_func1(angles[0], jid, proc_name);
                            exit(EXIT_SUCCESS);
                        }
                        else{
                            dup2(fd_array[2*i - 2], STDIN_FILENO);
                            close(fd_array[2*i + 1]);
                            wait(&child_status);
                            dup2(org_stdout, STDOUT_FILENO);
                            dup2(org_stdin, STDIN_FILENO);
                            dprintf(org_stdout, "Completed multiple pip redirection with right angle\n");
                        }
                    }

                    free(angles);
                    //rintf("Completed right angle redirection\n");
                }
                else{ // No right angle as last pipe
                    dprintf(org_stdout, "Regular piping\n");

                    pid_t pid;
                    int child_status;
                    if((pid = fork()) == -1){
                        dprintf(org_stdout, EXEC_ERROR, "Fork error");
                        close_pipes(fd_array, pos);
                        break;
                    }
                    else if(pid == 0){
                        dup2(fd_array[2*i - 2], STDIN_FILENO);
                        dup2(org_stdout, STDOUT_FILENO);
                        close(fd_array[2*i - 1]);
                        exec_func1(tokenized[i], jid, proc_name);
                        exit(EXIT_SUCCESS);
                    }
                    else{
                        dup2(fd_array[2*i], STDIN_FILENO);
                        close(fd_array[2*i + 1]);
                        wait(&child_status);
                    }

                    dup2(fd_array[2*i - 2], STDIN_FILENO);
                    dup2(org_stdout, STDOUT_FILENO);
                    dprintf(org_stdout, "Tokenized[%d] is %s\n", i, tokenized[i]);
                    exec_func1(tokenized[i], jid, proc_name);
                    dup2(org_stdin, STDIN_FILENO);
                    dprintf(org_stdout, "Completed multiple pip redirection\n");
                }
            }
            else{
                if(pos <= 2){
                    continue;
                }
                //printf("Middle pipes\n");
                if(memchr(tokenized[i], '<', strlen(tokenized[i])) != NULL || memchr(tokenized[i], '>', strlen(tokenized[i])) != NULL){
                    dprintf(org_stdout, SYNTAX_ERROR, "Invalid command");
                    close_pipes(fd_array, pos);
                    break;
                }
                //printf("Starting middle pipe redirection\n");
                pid_t pid;
                int child_status;
                if((pid = fork()) == -1){
                    dprintf(org_stdout, EXEC_ERROR, "Fork error");
                    close_pipes(fd_array, pos);
                    break;
                }
                else if(pid == 0){
                    dup2(fd_array[2*i - 2], STDIN_FILENO);
                    dup2(fd_array[2*i + 1], STDOUT_FILENO);
                    close(fd_array[2*i - 1]);
                    exec_func1(tokenized[i], jid, proc_name);
                    exit(EXIT_SUCCESS);
                }
                else{
                    dup2(fd_array[2*i], STDIN_FILENO);
                    close(fd_array[2*i + 1]);
                    wait(&child_status);
                }

                //printf("Completed middle pipe redirection\n");
            }
        }
        //printf("Closing pipes\n");

    } while(!exited);

    if(prev_dir != NULL){
        free(prev_dir);
    }
    free(cur_dir);
    free(jid);
    free(proc_name);

    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;

}