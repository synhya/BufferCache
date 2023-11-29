#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BLOCK_SIZE	4096
#define BLOCK_MAX_COUNT 1000
#define CACHE_SIZE 10

char *disk_buffer;
int disk_fd;

char *cache_buffer;
// Use Hash Table as Cache ?
// Pair<K, V> where K = block_nr, V = data
// 1. insert K to hash function
// 2. modulo new K by table size
// 3. table index = step 2 result, value = V
// 4. when finding value, hash(K) % table_size is the index.

int os_read(int block_nr, char *user_buffer)
{
    int ret;

    // implement BUFFERED_READ

    ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
    if (ret < 0)
        return ret;

    ret = read(disk_fd, disk_buffer, BLOCK_SIZE);
    if (ret < 0)
        return ret;

    memcpy(user_buffer, disk_buffer, BLOCK_SIZE);

    return ret;
}


int os_write(int block_nr, char *user_buffer)
{
    int ret;

    // implement BUFFERED_WRITE

    ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET);
    if (ret < 0)
        return ret;

    ret = write(disk_fd, user_buffer, BLOCK_SIZE);
    if (ret < 0)
        return ret;

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

    // for debugging
    if(truncate("diskfile", BLOCK_SIZE * BLOCK_MAX_COUNT) == -1)
    {
        printf("failed to truncate file \n");
    }

    disk_fd = open("diskfile", O_RDWR|O_DIRECT|O_CREAT, 0777);
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
    cache_buffer = malloc(BLOCK_SIZE * CACHE_SIZE);

    ret = lib_read(0, buffer);
    printf("nread: %d\n", ret);

    ret = lib_write(0, buffer);
    printf("nwrite: %d\n", ret);

    close(disk_fd);

    return 0;
}
