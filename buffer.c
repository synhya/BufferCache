#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "DataStructureLibrary/hashtable.h"
#include "DataStructureLibrary/linkedlist.h"

#define BLOCK_SIZE	4096
#define BLOCK_MAX_COUNT 1000
#define CACHE_SIZE 10

typedef struct CacheEntry{
    clock_t last_ref_time;      // access time for LRU
    int ref_count;          // reference count for LFU
    int dirty;              // dirty bit for delayed write
    int block_nr; 
    char buffer_data[BLOCK_SIZE];
    pthread_mutex_t lock;
} CacheEntry;

char *disk_buffer;
int disk_fd;

HashTable hash_table;

// need this in order to iterate through all cache entries in hash table
LinkedList cached_block_nr_list;

char* algorithm;

int LRU()
{
    return 0;
}

int LFU()
{
    return 0;
}

void write_to_disk(CacheEntry* entry) {
    int ret;

    pthread_mutex_lock(&(entry->lock));

    if(entry->dirty) {
        ret = lseek(disk_fd, entry->block_nr * BLOCK_SIZE, SEEK_SET);
        if(ret >= 0) {
            ret = write(disk_fd, entry->buffer_data, BLOCK_SIZE);
            if(ret >= 0) {
                entry->dirty = 0; 
            }
        }
    }

    pthread_mutex_unlock(&(entry->lock));
}

void* flush_thread(void* arg) {
    CacheEntry* entry = (CacheEntry*)arg;
    write_to_disk(entry);
    return NULL;
}

// delayed_write with thread
void start_flush_thread(CacheEntry* entry) {
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, flush_thread, (void*)entry) != 0) {
        perror("Failed to create the flush thread");
    }
    pthread_detach(thread_id); 
}


// call every time user asks to fill in buffer
// cache-hit -> update block_nr to front
// cache-miss -> add new block_nr (remove one if list is full)
void update_buffer_cache_state(int block_nr, char * buffer_data) {
    // update order of block_nr and return if list contains block_nr
    if(list_contains_item(&cached_block_nr_list, block_nr) == 1)
    {
        list_pop_item(&cached_block_nr_list, block_nr);
        list_insert_first(&cached_block_nr_list, block_nr);
        return;
    }
    // select victim if list is full
    // should remove from both list and cache
    else if (cached_block_nr_list.current_size >= cached_block_nr_list.capacity)
    {
        int target_block_nr;
        /****************** FIFO ******************/
        // remove last if reached limit
        if(strcmp(algorithm, "FIFO") == 0 || algorithm==NULL)
        {
            target_block_nr = list_pop_last(&cached_block_nr_list);
        }
        /****************** LRU ******************/
        // iterate all entries' last_ref_time and
        // erase the one with lowest time value
        else if(strcmp(algorithm, "LRU") == 0)
        {
            target_block_nr = LRU();
            list_pop_item(&cached_block_nr_list, target_block_nr);
        }
        /****************** LFU ******************/
        // iterate all entries' ref_count value and
        // erase the one with lowest time value
        else if(strcmp(algorithm, "LFU") == 0)
        {
            target_block_nr = LFU();
            list_pop_item(&cached_block_nr_list, target_block_nr);
        }

        CacheEntry* entry = (CacheEntry*)ht_lookup(&hash_table, &target_block_nr);
        if(entry->dirty == 1)
        {
            start_flush_thread(entry);
        }
        ht_erase(&hash_table, &target_block_nr);
    }

    // insert block_nr to list, cache_entry to hash_table
    list_insert_first(&cached_block_nr_list, block_nr);

    CacheEntry cache_entry;
    cache_entry.last_ref_time = clock();
    cache_entry.ref_count = 0;
    cache_entry.dirty = 0;
    memcpy(cache_entry.buffer_data, buffer_data, BLOCK_SIZE);
    ht_insert(&hash_table, &block_nr, &cache_entry);
}

int buffered_read(int block_nr, char *user_buffer)
{
    if(ht_contains(&hash_table, &block_nr)) {
        CacheEntry* cache_entry = (CacheEntry*)ht_lookup(&hash_table, &block_nr);

        cache_entry->ref_count += 1;
        cache_entry->last_ref_time = clock();
        memcpy(user_buffer, cache_entry->buffer_data, BLOCK_SIZE);

        return 0;
    }

    return -1;
}

int buffered_write(int block_nr, char *user_buffer)
{
    if(ht_contains(&hash_table, &block_nr)) {
        CacheEntry* cache_entry = (CacheEntry*)ht_lookup(&hash_table, &block_nr);

        cache_entry->ref_count += 1;
        cache_entry->last_ref_time = clock();
        cache_entry->dirty = 1;
        memcpy(cache_entry->buffer_data, user_buffer, BLOCK_SIZE);

        return 0;
    }

    return -1;
}


int os_read(int block_nr, char *user_buffer)
{
    int ret;

    // implement BUFFERED_READ
    ret = buffered_read(block_nr, user_buffer);

    /****************** cache miss ******************/
    if(ret == -1) {
        ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
        if (ret < 0)
            return ret;

        ret = read(disk_fd, disk_buffer, BLOCK_SIZE);
        if (ret < 0)
            return ret;

        memcpy(user_buffer, disk_buffer, BLOCK_SIZE);
    }

    update_buffer_cache_state(block_nr, user_buffer);

    return ret;
}

int os_write(int block_nr, char *user_buffer)
{
    int ret;

    // implement BUFFERED_WRITE
    ret = buffered_write(block_nr, user_buffer);

    /****************** cache miss ******************/
    if(ret == -1) {
        ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
        if (ret < 0)
            return ret;

        ret = write(disk_fd, user_buffer, BLOCK_SIZE);
        if (ret < 0)
            return ret;
    }

    update_buffer_cache_state(block_nr, user_buffer);

    return ret;
}

int lib_read(int block_nr, char *user_buffer)
{
    int ret;
    ret = os_read(block_nr, user_buffer);

    return ret;
}

int lib_write(int block_nr, char *user_buffer)
{
    int ret;
    ret = os_write(block_nr, user_buffer);

    return ret;
}

int init()
{
    disk_buffer = aligned_alloc(BLOCK_SIZE, BLOCK_SIZE);
    if (disk_buffer == NULL)
        return -errno;

    printf("disk_buffer: %p\n", disk_buffer);

    /////////////////////////////////////////////////////////////////////
    /// create dummy file for debugging (comment out if don't need)
    if(truncate("diskfileD", BLOCK_SIZE * BLOCK_MAX_COUNT) == -1)
    {
        printf("failed to truncate file \n");
    }
    /////////////////////////////////////////////////////////////////////

    disk_fd = open("diskfileD", O_RDWR|O_DIRECT|O_CREAT, 0777);
    if (disk_fd < 0)
    {
        printf("failed to open file \n");
        return disk_fd;
    }
}

int main (int argc, char *argv[])
{
    char *buffer;
    int ret;


    init();

    buffer = malloc(BLOCK_SIZE);

    ht_setup(&hash_table, sizeof(int), sizeof(CacheEntry), CACHE_SIZE);
    list_setup(&cached_block_nr_list, CACHE_SIZE);
    algorithm = argv[1];

    ret = lib_read(0, buffer);
    printf("nread: %d\n", ret);

    ret = lib_write(0, buffer);
    printf("nwrite: %d\n", ret);

    ht_clear(&hash_table);
    ht_destroy(&hash_table);

    list_clear(&cached_block_nr_list);

    close(disk_fd);
    free(buffer);

    return 0;
}
