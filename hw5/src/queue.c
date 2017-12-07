#include "queue.h"
#include "csapp.h"

queue_t *create_queue(void) {
    queue_t* queue = (queue_t*)(calloc(1, sizeof(queue_t)));
    queue->front = NULL;
    queue->rear = NULL;
    Sem_init(&(queue->items), 0, 0);
    int ret;
    if ((ret = pthread_mutex_init(&(queue->lock), NULL) != 0))
        posix_error(ret, "pthread_mutex_init error");
    queue->invalid = false;
    return queue;
}

bool invalidate_queue(queue_t *self, item_destructor_f destroy_function) {
    if(self == NULL){
        errno = EINVAL;
        return false;
    }

    pthread_mutex_lock(&(self->lock)); // Locks the queue
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->lock));
        return false;
    }
    self->invalid = true;
    queue_node_t* cur_node = self->front;
    while(cur_node != NULL){
        (destroy_function)(cur_node->item);
        free(cur_node);
        cur_node = cur_node-> next;
    }
    pthread_mutex_unlock(&(self->lock)); // Unlocks the queue

    return true;
}

bool enqueue(queue_t *self, void *item) {
    if(self == NULL || item == NULL){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&(self->lock)); // Locks the queue
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->lock));
        return false;
    }
    queue_node_t* new_node = (queue_node_t*)(calloc(1, sizeof(queue_node_t)));
    new_node->item = item;
    new_node->next = NULL;
    if(self->rear == NULL){
        self->front = new_node;
    }
    else{
        self->rear->next = new_node;
    }
    self->rear = new_node;
    pthread_mutex_unlock(&(self->lock)); // Unlocks the queue
    V(&(self->items)); // Increment number of items

    return true;
}

void *dequeue(queue_t *self) {
    if(self == NULL){
        errno = EINVAL;
        return NULL;
    }

    pthread_mutex_lock(&(self->lock)); // Locks the queue
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->lock));
        return NULL;
    }
    pthread_mutex_unlock(&(self->lock));

    P(&(self->items)); // Wait for an item
    pthread_mutex_lock(&(self->lock)); // Locks the queue
    queue_node_t* old_front = self->front;
    self->front = old_front->next;
    if(old_front == self->rear){ // If there was only one item in the queue
        self->rear = NULL;
    }
    void* item = old_front->item;
    free(old_front);
    pthread_mutex_unlock(&(self->lock)); // Unlocks the queue

    return item;
}
