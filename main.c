#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "btree.h"

void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head);
void print_file_header(struct file_header *head);

int main(void) {
    struct file_header head;
    uint8_t *raw = must_malloc(100);
    int fd = open("test.db", O_RDONLY);
    assert(fd >= 0);
    ssize_t bytes_read = read(fd, raw, 100);
    assert(bytes_read == 100);

    /* parse and process the header */
    unpack_file_header(raw,100,&head);
    free(raw);
    print_file_header(&head);

    int close_ret = close(fd);
    assert(close_ret >= 0);
}


