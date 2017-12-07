#include "cream.h"
#include "utils.h"
#include "csapp.h"
#include <string.h>
#include <stdio.h>
#include "queue.h"
#ifdef EC
#include "extracredit.h"
#else
#include "hashmap.h"
#endif
#include <signal.h>
#include <errno.h>

void print_help(){
    printf("./cream [-h] NUM_WORKERS PORT_NUMBER MAX_ENTRIES\n"
            "-h                 Displays this help menu and returns EXIT_SUCCESS.\n"
            "NUM_WORKERS        The number of worker threads used to service requests.\n"
            "PORT_NUMBER        Port number to listen on for incoming connections.\n"
            "MAX_ENTRIES        The maximum number of entries that can be stored in `cream`'s underlying data store.\n");
}

void destroy(map_key_t key, map_val_t val){
    //printf("Destroy free\n");
    if(key.key_base != NULL)
        //printf("Free key_base\n");
        free(key.key_base);
    if(val.val_base != NULL)
        //printf("Free val_base\n");
        free(val.val_base);
}

void destroy_queue(void* item){
    int* connfd = (int*)item;
    Close(*connfd);
    free(connfd);
}

queue_t* request_queue;
hashmap_t* data_map;


void* spawn_worker(void* arg){
    while(1){
        int* connfd = (int*)dequeue(request_queue); // Wait for a connected file descriptor
        //printf("Dequeuing connfd %d\n", *connfd);
        errno = 0;
        //Service Client
        rio_t rio;
        char* read_head[sizeof(request_header_t)];
        //printf("Read init\n");
        Rio_readinitb(&rio, *connfd);
        //printf("Allocating res_head memory\n");
        response_header_t* res_head = calloc(1, sizeof(response_header_t));
        //printf("Read request header\n");
        Rio_readnb(&rio, read_head, sizeof(request_header_t));
        //int n;
        //while((n = Rio_readnb(&rio, read_head, sizeof(request_header_t))) != 0){
        //printf("While\n");
        while(1){
            if(errno != EPIPE){
                request_header_t* req_head = (request_header_t*)((void*)read_head);
                request_codes request = req_head->request_code;
                uint32_t key_size = req_head->key_size;
                uint32_t val_size = req_head->value_size;
                switch(request){
                    case(PUT):
                        //printf("PUT with connfd %d\n", *connfd);
                        if(key_size < MIN_KEY_SIZE || key_size > MAX_KEY_SIZE || val_size < MIN_VALUE_SIZE || val_size > MAX_VALUE_SIZE){

                            res_head->response_code = BAD_REQUEST;
                            res_head->value_size = 0;
                        }
                        else{

                            map_key_t* key = (map_key_t*)(calloc(1, sizeof(map_key_t)));
                            key->key_base = calloc(key_size + 1, sizeof(char));
                            map_val_t* val = (map_val_t*)(calloc(1, sizeof(map_val_t)));
                            val->val_base = calloc(val_size + 1, sizeof(char));
                            Rio_readnb(&rio, key->key_base, key_size);
                            if(errno == EPIPE){
                                //printf("Free 1\n");
                                free(key);
                                free(val);
                                break;
                            }
                            Rio_readnb(&rio, val->val_base, val_size);
                            if(errno == EPIPE){
                                //printf("Free 2\n");
                                free(key);
                                free(val);
                                break;
                            }
                            key->key_len = key_size;
                            val->val_len = val_size;
                            bool success = put(data_map, *key, *val, true);
                            if(!success){
                                res_head->response_code = BAD_REQUEST;
                            }
                            else{
                                res_head->response_code = OK;
                            }
                            res_head->value_size = 0;
                            //printf("Free 3\n");
                            free(key);
                            free(val);
                        }
                        Rio_writen(*connfd, (void*)res_head, sizeof(response_header_t));
                        if(errno == EPIPE){
                            break;
                        }
                        break;
                    case(GET):
                        if(key_size < MIN_KEY_SIZE || key_size > MAX_KEY_SIZE){
                            res_head->response_code = BAD_REQUEST;
                            res_head->value_size = 0;
                        }
                        else{
                            //printf("GET with connfd %d\n", *connfd);
                            map_key_t* key = (map_key_t*)(calloc(1, sizeof(map_key_t)));
                            key->key_base = calloc(key_size + 1, sizeof(char));
                            Rio_readnb(&rio, key->key_base, key_size);
                            if(errno == EPIPE){
                                //printf("Free 4\n");
                                free(key);
                                break;
                            }
                            key->key_len = key_size;
                            map_val_t ret = get(data_map, *key);
                            if(ret.val_base == NULL){
                                res_head->response_code = NOT_FOUND;
                                res_head->value_size = 0;
                            }
                            else{
                                res_head->response_code = OK;
                                res_head->value_size = ret.val_len;
                            }
                            Rio_writen(*connfd, (void*)res_head, sizeof(response_header_t));
                            if(errno == EPIPE){
                                //printf("Free 5\n");
                                free(key);
                                break;
                            }
                            Rio_writen(*connfd, ret.val_base, res_head->value_size);
                            //printf("Free 6\n");
                            free(key);
                            //printf("Freed 6\n");
                        }
                        break;
                    case(EVICT):
                        if(key_size < MIN_KEY_SIZE || key_size > MAX_KEY_SIZE){
                            res_head->response_code = BAD_REQUEST;
                            res_head->value_size = 0;
                        }
                        else{
                            map_key_t* key = (map_key_t*)(calloc(1, sizeof(map_key_t)));
                            key->key_base = calloc(key_size + 1, sizeof(char));
                            Rio_readnb(&rio, (char*)(key->key_base), key_size);
                            if(errno == EPIPE){
                                //printf("Free 7\n");
                                free(key);
                                break;
                            }
                            key->key_len = key_size;
                            delete(data_map, *key);
                            res_head->response_code = OK;
                            res_head->value_size = 0;
                            //printf("Free 8\n");
                            free(key);
                        }
                        Rio_writen(*connfd, (void*)res_head, sizeof(response_header_t));
                        break;
                    case(CLEAR):
                        clear_map(data_map);
                        res_head->response_code = OK;
                        res_head->value_size = 0;
                        Rio_writen(*connfd, (void*)res_head, sizeof(response_header_t));
                        break;
                    default:
                        res_head->response_code = UNSUPPORTED;
                        res_head->value_size = 0;
                        Rio_writen(*connfd, (void*)res_head, sizeof(response_header_t));
                        break;
                }
            }

            break;
        }

        Close(*connfd);
        //printf("Free connfd\n");
        free(connfd);
        //printf("Freed connfd\n");
        free(res_head);
        //printf("Freed res head\n");
    }

    return NULL;
}

int main(int argc, char *argv[]) {

    // Signal handling
    void sigpipe_handler(int sig){

    }

    Signal(SIGPIPE, sigpipe_handler);

    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    if(argc == 1){
        print_help();
        exit(EXIT_SUCCESS);
    }

    int i = 1;
    for(; i < argc; i++){ // Check for help flag
        if(strcmp(argv[i], "-h") == 0){
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    if(argc != 4){
        exit(EXIT_FAILURE);
    }

    int num_workers = atoi(argv[1]);
    if(num_workers < 1){
        exit(EXIT_FAILURE);
    }
    char* port_number = argv[2];
    int max_entries = atoi(argv[3]);
    if(max_entries < 1){
        exit(EXIT_FAILURE);
    }
    request_queue = create_queue();
    data_map = create_map(max_entries, jenkins_one_at_a_time_hash, destroy);
    int listenfd = Open_listenfd(port_number);

    pthread_t thread_ids[num_workers];
    int index = 0;
    for(; index < num_workers; index++){ // Spawn worker threads
        Pthread_create(&thread_ids[index], NULL, spawn_worker, NULL);
    }

    while(1){
        int* connfd = calloc(1, sizeof(int));
        *connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        //printf("Enqueueing connfd %d\n", *connfd);
        enqueue(request_queue, connfd);
    }
    exit(EXIT_SUCCESS);
}
