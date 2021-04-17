#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "btree.h"

void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head);
void print_file_header(struct file_header *head);
struct btree_page *unpack_btree_page(uint8_t *raw, uint32_t size, uint32_t page_number);
void print_btree_page(struct btree_page *bpage);
void free_btree_page(struct btree_page *bpage);

int main(int argc, char **argv) {
    int pagenumber = 0;

    switch (argc) {
        case 1:
            break;
        case 2:
            pagenumber = atoi(argv[1]);
            break;
        default:
            fprintf(stderr, "Usage: %s ,pagenumber>\n", argv[0]);
            exit(1);
    }
    if (pagenumber < 0) {
        fprintf(stderr, "Page number must be >= 0\n");
        exit(1);
    }

    struct file_header head;
    uint8_t *raw = must_malloc(FILE_HEADER_SIZE);
    int fd = open("test.db", O_RDONLY);
    assert(fd >= 0);
    ssize_t bytes_read = read(fd, raw, FILE_HEADER_SIZE);
    assert(bytes_read == FILE_HEADER_SIZE);

    /* parse and process the header */
    unpack_file_header(raw,FILE_HEADER_SIZE,&head);
    free(raw);

    if (pagenumber == 0) {
        print_file_header(&head);
        int close_ret = close(fd);
        assert(close_ret >= 0);
        return 0;
    }

    if (pagenumber == 1 ) {
        lseek(fd,FILE_HEADER_SIZE ,SEEK_SET);
    } else {
        int page_offset = head.page_size*(pagenumber-1);
        printf("page offset         : %d\n", page_offset);
        lseek(fd,page_offset,SEEK_SET);
    }
    uint8_t *raw2 = must_malloc(head.page_size);
    ssize_t bytes_read2 = read(fd,raw2,head.page_size);
    assert(bytes_read2 == head.page_size);
    struct btree_page *bpage = unpack_btree_page(raw2,head.page_size,pagenumber);
    printf("\n");
    print_btree_page(bpage);

    int close_ret = close(fd);
    assert(close_ret >= 0);
    free(raw2);
    free_btree_page(bpage);

    return 0;
}
