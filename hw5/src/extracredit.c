#include "utils.h"
#include "csapp.h"

#define MAP_KEY(base, len) (map_key_t) {.key_base = base, .key_len = len}
#define MAP_VAL(base, len) (map_val_t) {.val_base = base, .val_len = len}
#define MAP_NODE(key_arg, val_arg, tombstone_arg) (map_node_t) {.key = key_arg, .val = val_arg, .tombstone = tombstone_arg}

hashmap_t *create_map(uint32_t capacity, hash_func_f hash_function, destructor_f destroy_function) {
    if(capacity == 0){
        return NULL;
    }
    hashmap_t* hashmap;
    if((hashmap = (hashmap_t*)(calloc(1, sizeof(hashmap_t)))) == NULL)
        return NULL;
    hashmap->capacity = capacity;
    hashmap->size = 0;
    hashmap->nodes = (map_node_t*)(calloc(capacity, sizeof(map_node_t)));
    hashmap->hash_function = hash_function;
    hashmap->destroy_function = destroy_function;
    hashmap->num_readers = 0;
    int ret;
    if ((ret = pthread_mutex_init(&(hashmap->write_lock), NULL) != 0))
        posix_error(ret, "pthread_mutex_init error");
    if ((ret = pthread_mutex_init(&(hashmap->fields_lock), NULL) != 0))
        posix_error(ret, "pthread_mutex_init error");
    hashmap->invalid = false;
    return hashmap;
}

bool put(hashmap_t *self, map_key_t key, map_val_t val, bool force) {
    if(self == NULL || key.key_base == NULL || key.key_len == 0 || val.val_base == NULL || val.val_len == 0){
        errno = EINVAL;
        return false;
    }

    pthread_mutex_lock(&(self->fields_lock));
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->fields_lock));
        return false;
    }
    pthread_mutex_unlock(&(self->fields_lock));

    pthread_mutex_lock(&(self->write_lock));
    int index = get_index(self, key);
    map_node_t* new_node = self->nodes + index;
    int counter = 0, lru_index = 0;
    /*struct timeval end;
    gettimeofday(&end, NULL);*/
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    long lru_time = 0.0, cur_node_time;
    cur_node_time = (end.tv_sec - new_node->start.tv_sec) * 1000000;
    cur_node_time += (end.tv_nsec - new_node->start.tv_nsec) / 1000; // Calculate elapsed time in microseconds
    while(!(new_node->key.key_len == 0 || new_node->tombstone || cur_node_time >= TTL * 1000000)
            && counter < self->capacity){ // While the next node is not null, is not a tombstone, and the counter is less than the capacity
        if(new_node->key.key_len == key.key_len && memcmp(new_node->key.key_base, key.key_base, key.key_len) == 0){ // If the key exists, update the value
            if(new_node->key.key_len != -1){
                (self->destroy_function)(new_node->key, new_node->val);
                new_node->key.key_len = -1;
            }
            new_node->key = key;
            new_node->val = val;
            new_node->tombstone = false;
            //gettimeofday(&(new_node->start), NULL);
            clock_gettime(CLOCK_MONOTONIC,&(new_node->start));
            pthread_mutex_unlock(&(self->write_lock));
            pthread_mutex_lock(&(self->fields_lock));
            self->size += 1; // Update size
            pthread_mutex_unlock(&(self->fields_lock));
            return true;
        }
        if(cur_node_time > lru_time){ // Test for LRU
            lru_time = cur_node_time;
            lru_index = index;
        }
        index = (index + 1) % self->capacity;
        new_node = self->nodes + index;
        cur_node_time = (end.tv_sec - new_node->start.tv_sec) * 1000000;
        cur_node_time += (end.tv_nsec - new_node->start.tv_nsec) / 1000; // Calculate elapsed time in microseconds
        counter++;
    }
    if(counter == self->capacity){ // If the map is full
        if(force){ // Size stays the same
            new_node = self->nodes + lru_index; // LRU index
            if(new_node->key.key_len != -1){
                (self->destroy_function)(new_node->key, new_node->val);
                new_node->key.key_len = -1;
            }
            new_node->key = key;
            new_node->val = val;
            new_node->tombstone = false;
            //gettimeofday(&(new_node->start), NULL);
            clock_gettime(CLOCK_MONOTONIC,&(new_node->start));
            pthread_mutex_unlock(&(self->write_lock));
            pthread_mutex_lock(&(self->fields_lock));
            self->size += 1; // Update size
            pthread_mutex_unlock(&(self->fields_lock));
            return true;
        }
        else{
            errno = ENOMEM;
            pthread_mutex_unlock(&(self->write_lock));
            pthread_mutex_lock(&(self->fields_lock));
            self->size += 1; // Update size
            pthread_mutex_unlock(&(self->fields_lock));
            return false;
        }
    }

    if(new_node->tombstone && new_node->key.key_len != -1){
        (self->destroy_function)(new_node->key, new_node->val);
        new_node->key.key_len = -1;
    }
    new_node->key = key;
    new_node->val = val;
    new_node->tombstone = false;
    //gettimeofday(&(new_node->start), NULL);
    clock_gettime(CLOCK_MONOTONIC,&(new_node->start));
    pthread_mutex_unlock(&(self->write_lock));

    pthread_mutex_lock(&(self->fields_lock));
    self->size += 1; // Update size
    pthread_mutex_unlock(&(self->fields_lock));

    return true;
}

map_val_t get(hashmap_t *self, map_key_t key) {
    if(self == NULL || key.key_base == NULL || key.key_len == 0){
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    pthread_mutex_lock(&(self->fields_lock));
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->fields_lock));
        return MAP_VAL(NULL, 0);
    }
    self->num_readers++;
    if(self->num_readers == 1){
        pthread_mutex_lock(&(self->write_lock));
    }
    pthread_mutex_unlock(&(self->fields_lock));
    int index = get_index(self, key);
    int counter = 0;
    map_node_t* cur_node = self->nodes + index;
    /*struct timeval end;
    gettimeofday(&end, NULL);*/
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    long cur_node_time;
    cur_node_time = (end.tv_sec - cur_node->start.tv_sec) * 1000000;
    cur_node_time += (end.tv_nsec - cur_node->start.tv_nsec) / 1000; // Calculate elapsed time in microseconds
    while(!((cur_node->key.key_len != 0 // While the key length is zero
            && cur_node->key.key_len == key.key_len // The indexed key length is not equal to the given key length
            && memcmp(cur_node->key.key_base, key.key_base, key.key_len) == 0 // The index based is not equal to the given base
            && !cur_node->tombstone // The current node is a tombstone
            && cur_node_time < TTL * 1000000) // The node has been in the map for more than the TTL
            || counter >= self->capacity)){ // OR the counter is less than the capacity
        index = (index + 1) % self->capacity;
        cur_node = self->nodes + index;
        cur_node_time = (end.tv_sec - cur_node->start.tv_sec) * 1000000;
        cur_node_time += (end.tv_nsec - cur_node->start.tv_nsec) / 1000; // Calculate elapsed time in microseconds
        counter++;
    }

    pthread_mutex_lock(&(self->fields_lock));
    //gettimeofday(&(cur_node->start), NULL);
    //clock_gettime(CLOCK_MONOTONIC,&(cur_node->start));
    printf("Elapsed time is %lu\n", cur_node_time);
    self->num_readers--;
    if(self->num_readers == 0){
        pthread_mutex_unlock(&(self->write_lock));
    }
    //gettimeofday(&(cur_node->start), NULL); // Update clock
    clock_gettime(CLOCK_MONOTONIC,&(cur_node->start));
    pthread_mutex_unlock(&(self->fields_lock));

    if(counter == self->capacity || cur_node->tombstone || cur_node_time >= TTL * 1000000){
        if(cur_node_time >= TTL * 1000000){
            cur_node->tombstone = true;
        }
        return MAP_VAL(NULL, 0);
    }
    return cur_node->val;
}

map_node_t delete(hashmap_t *self, map_key_t key) {
    if(self == NULL || key.key_base == NULL || key.key_len == 0){
        errno = EINVAL;
        return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }

    pthread_mutex_lock(&(self->fields_lock));
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->fields_lock));
        return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }
    pthread_mutex_unlock(&(self->fields_lock));

    pthread_mutex_lock(&(self->write_lock));
    int index = get_index(self, key);
    int counter = 0;
    map_node_t* cur_node = self->nodes + index;
    while(!((cur_node->key.key_len != 0
            && cur_node->key.key_len == key.key_len
            && memcmp(cur_node->key.key_base, key.key_base, key.key_len) == 0)
            || counter >= self->capacity)){
        index = (index + 1) % self->capacity;
        cur_node = self->nodes + index;
        counter++;
    }
    if(counter != self->capacity){
        cur_node->tombstone = true;
    }
    pthread_mutex_unlock(&(self->write_lock));

    if(counter == self->capacity){
        return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }

    pthread_mutex_lock(&(self->fields_lock));
    self->size -= 1;
    pthread_mutex_unlock(&(self->fields_lock));

    return *cur_node;
}

bool clear_map(hashmap_t *self) {
    if(self == NULL){
        errno = EINVAL;
        return false;
    }

    pthread_mutex_lock(&(self->fields_lock));
    if(self->invalid){
        errno = EINVAL;
        pthread_mutex_unlock(&(self->fields_lock));
        return false;
    }
    pthread_mutex_unlock(&(self->fields_lock));

    int index = 0;
    pthread_mutex_lock(&(self->write_lock));
    while(index < self->capacity){
        map_node_t* cur_node = self->nodes + index;
        cur_node->tombstone = true;
        if(cur_node->key.key_len != -1){
            (self->destroy_function)(cur_node->key, cur_node->val);
            cur_node->key.key_len = -1;
        }
        index++;
    }
    pthread_mutex_unlock(&(self->write_lock));

    pthread_mutex_lock(&(self->fields_lock));
    self->size = 0;
    pthread_mutex_unlock(&(self->fields_lock));

    return true;
}

bool invalidate_map(hashmap_t *self) {
    if(self == NULL || self->invalid){
        errno = EINVAL;
        return false;
    }
    clear_map(self);
    pthread_mutex_lock(&(self->fields_lock));
    pthread_mutex_lock(&(self->write_lock));
    free(self->nodes);
    self->invalid = true;
    pthread_mutex_unlock(&(self->write_lock));
    pthread_mutex_unlock(&(self->fields_lock));
    return true;
}
