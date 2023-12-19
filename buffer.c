#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include "DataStructureLibrary/hashtable.h"
#include "DataStructureLibrary/linkedlist.h"

#define BLOCK_SIZE	4096
#define BLOCK_MAX_COUNT 100
#define CACHE_SIZE 10

typedef struct CacheEntry{
    clock_t last_ref_time;      // access time for least_recently_used
    int ref_count;          // reference count for least_frequently_used
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
clock_t start_time;

int* block_access_sequence;
int hit_counter;

double average_hit_access_time;
double average_miss_access_time;

double box_muller_distribution() {
    double x, y, z = -99;
    double limitValue = 2.5;

    // log x가 무한히 작아질 수 있어서 대충 2.5에서 자름
    while(z < -limitValue || z > limitValue) {
        while((x = (double)random() / RAND_MAX) == 0) {}

        y = (double)random() / RAND_MAX;

        z = sqrt(-2 * log(x)) * cos(2 * M_PI * y);
    }
    // -limitValue ~ limitValue to 0 ~ 1
    double ret = (z + limitValue) / (2 * limitValue);

    return ret;
}

int least_recently_used()
{
    Node* ptr = cached_block_nr_list.head;
    CacheEntry* entry;
    clock_t least_recent_time = start_time;
    int target_idx = -1;

    for(;ptr != NULL; ptr = ptr->next) {
        entry = (CacheEntry*)ht_lookup(&hash_table, &ptr->value);
        clock_t last_ref_time = entry->last_ref_time;
        //가장 오래전 참조된 부분을 찾음
        if(last_ref_time < least_recent_time) {
            least_recent_time = last_ref_time;
            target_idx = ptr->value;
        }
    }

    return target_idx;
}

int least_frequently_used()
{
    Node* ptr = cached_block_nr_list.head;
    //캐시 엔트리가 비어있는지 확인하기 위해 선언과 함께 초기화
    CacheEntry* entry = (CacheEntry*)ht_lookup(&hash_table, &ptr->value);

    //캐시 엔트리가 유효한지 확인
    if (entry == NULL) {
        return -1;
    }

    int min=9999999;
    int target_idx = -1;

    for(;ptr != NULL; ptr = ptr->next) {
        entry = (CacheEntry*)ht_lookup(&hash_table, &ptr->value);
        int ref_count = entry->ref_count;

        if(min > ref_count) {
            min = ref_count;
            target_idx = ptr->value;
        }
    }

    return target_idx;
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

    // segfault error occurs if erase from hash_table (bug)
//    ht_erase(&hash_table, &entry->block_nr);

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

// insert block_nr to list, cache_entry to hash_table
void create_cache_entry(int block_nr, char * buffer_data)
{
    list_insert_first(&cached_block_nr_list, block_nr);

    CacheEntry cache_entry;
    cache_entry.block_nr = block_nr;
    cache_entry.last_ref_time = clock();
    cache_entry.ref_count = 0;
    cache_entry.dirty = 0;
    memcpy(cache_entry.buffer_data, buffer_data, BLOCK_SIZE);
    ht_insert(&hash_table, &block_nr, &cache_entry);
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
        if(algorithm==NULL || strcmp(algorithm, "FIFO") == 0)
        {
            target_block_nr = list_pop_last(&cached_block_nr_list);
        }
        /****************** least_recently_used ******************/
        // iterate all entries' last_ref_time and
        // erase the one with lowest time value
        else if(strcmp(algorithm, "least_recently_used") == 0)
        {
            target_block_nr = least_recently_used();
            list_pop_item(&cached_block_nr_list, target_block_nr);
        }
        /****************** least_frequently_used ******************/
        // iterate all entries' ref_count value and
        // erase the one with lowest time value
        else if(strcmp(algorithm, "least_frequently_used") == 0)
        {
            target_block_nr = least_frequently_used();
            list_pop_item(&cached_block_nr_list, target_block_nr);
        }

        // problem part!

        CacheEntry* entry = (CacheEntry*)ht_lookup(&hash_table, &target_block_nr);
        if(entry->dirty == 1)
        {
            start_flush_thread(entry);
        }

        // segfault error occurs if erase from hash_table (bug)
//      ht_erase(&hash_table, &target_block_nr);

    }

    create_cache_entry(block_nr, buffer_data);
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
    clock_t before_t = clock();

    // implement BUFFERED_READ
    if((ret = buffered_read(block_nr, user_buffer)) == 0) {
        hit_counter++;

        clock_t after_t = clock();
        average_hit_access_time += (double)(clock() - before_t) / CLOCKS_PER_SEC;
    }

    /****************** cache miss ******************/
    if(ret == -1) {
        ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
        if (ret < 0)
            return ret;

        ret = read(disk_fd, disk_buffer, BLOCK_SIZE);
        if (ret < 0)
            return ret;

        memcpy(user_buffer, disk_buffer, BLOCK_SIZE);

        average_miss_access_time += (double)(clock() - before_t) / CLOCKS_PER_SEC;
    }

    update_buffer_cache_state(block_nr, user_buffer);

    return ret;
}

int os_write(int block_nr, char *user_buffer)
{
    int ret;

    clock_t before_t = clock();

    // implement BUFFERED_WRITE
    if((ret = buffered_write(block_nr, user_buffer)) == 0) {
        hit_counter++;

        average_hit_access_time += (double)(clock() - before_t) / CLOCKS_PER_SEC;
    }

    /****************** cache miss ******************/
    if(ret == -1) {
        ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
        if (ret < 0)
            return ret;

        ret = write(disk_fd, user_buffer, BLOCK_SIZE);
        if (ret < 0)
            return ret;

        average_miss_access_time += (double)(clock() - before_t) / CLOCKS_PER_SEC;
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

    average_hit_access_time = 0;
    average_miss_access_time = 0;

    init();

    buffer = malloc(BLOCK_SIZE);

    ht_setup(&hash_table, sizeof(int), sizeof(CacheEntry), BLOCK_MAX_COUNT * 5);
    list_setup(&cached_block_nr_list, CACHE_SIZE);
    algorithm = argv[1];
    start_time = clock();

    unsigned int seed = time(NULL);
    srand(seed);

    // generate access sequence
    int access_multiplier = 5;
    int access_count = BLOCK_MAX_COUNT * access_multiplier;
    block_access_sequence = malloc(sizeof(int) * access_count);
    for(int i = 0; i < access_count; i++) {
        double dist_value = box_muller_distribution();
        block_access_sequence[i] = (int)floor(dist_value * BLOCK_MAX_COUNT);
    }
    hit_counter = 0;

    // access block
    for(int i = 0; i < access_count; i++) {
        if((double)random()/RAND_MAX > 0.5) {
            ret = lib_read(block_access_sequence[i], buffer);
        } else {
            ret = lib_write(block_access_sequence[i], buffer);
        }
    }

    // print result
    printf("total access count: %d\n", access_count);
    printf("total hit count: %d\n", hit_counter);
    printf("hit ratio: %lf\n", (double)hit_counter / (double)access_count);
    printf("average hit access time : %lf\n", average_hit_access_time / (double)hit_counter);
    printf("average miss access time : %lf\n", average_miss_access_time / (double)(hit_counter - access_count));

    ht_clear(&hash_table);

    list_clear(&cached_block_nr_list);

    close(disk_fd);
    free(buffer);
    free(block_access_sequence);

    return 0;
}
