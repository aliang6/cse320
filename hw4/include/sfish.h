#ifndef SFISH_H
#define SFISH_H

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"

#endif

char* tokenize_input(char* input);
void builtin_help();
void builtin_exit();
char* builtin_cd(char* input, char* cur_dir, char* prev_dir); // Returns path of previous directory
void builtin_pwd(char* input);
void exec_func(char* input);
//void exec_func1(char* input, int* jid, char** proc_name, int jid_cur, int jid_max, int );
void close_pipes(int* fd_array, int size);

void builtin_jobs(int* jid, char* proc_name, int jid_max);

void builtin_fg(char* tok_input, int* pids);

void builtin_kill(char* tok_input, int* pids);

void builtin_color();