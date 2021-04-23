#include <assert.h>
#include "btree.h"
#include <fcntl.h>
#include <inttypes.h>
#include "serializer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    uint32_t page_number = 0;
    int64_t start_key = -1;
    int64_t end_key = -1;

    if (argc >= 2) {
        page_number = atoi(argv[1]);
        if (page_number < 1) {
            fprintf(stderr, "page number must be > 0\n");
            return 1;
        }
    }

    if (argc >= 3) {
        start_key = atoi(argv[2]);
        if (start_key < 1) {
            fprintf(stderr, "start key must be > 0\n");
            return 1;
        }
    }

    if (argc == 4) {
        end_key = atoi(argv[3]);
        if (end_key < 1) {
            fprintf(stderr, "end key must be > 0\n");
            return 1;
        }
    }

    if (argc > 4) {
        fprintf(stderr, "Usage: %s [page number [start key [end key]]]\n", argv[0]);
        return 1;
    }

    struct file_header head;
    int fd = open("test.db", O_RDONLY);
    assert(fd >= 0);
    uint8_t *raw = must_malloc(FILE_HEADER_SIZE);
    ssize_t ret = read(fd, raw, FILE_HEADER_SIZE);
    assert(ret == FILE_HEADER_SIZE);

    /* read and unpack the header */
    unpack_file_header(raw, FILE_HEADER_SIZE, &head);
    free(raw);

    if (page_number == 0) {
        print_file_header(&head);
        int close_ret = close(fd);
        assert(close_ret >= 0);
        return 0;
    }

    if (start_key < 1) {
        raw = must_malloc(head.page_size);

        /* read the requested block */
        off_t lseek_ret = lseek(fd, (page_number - 1) * head.page_size, SEEK_SET);
        assert(lseek_ret == (page_number - 1) * head.page_size);
        ret = read(fd, raw, head.page_size);
        assert(ret == head.page_size);

        struct page *page = unpack_page(raw, head.page_size, page_number);

        print_page(page);

        free_page(page);
        free(raw);
    } else {
        if (end_key < 1)
            end_key = start_key + 1;
        struct query_iterator *iter = query_seek(&head, fd, page_number, start_key);
        if (iter == NULL)  {
            printf("not found\n");
        } else {
            for (;;) {
                struct cell *cell = query_get(iter);
                int64_t key = 0;
                if (cell->page_type == TABLE_LEAF)
                    key = cell->key;
                else if (cell->page_type == INDEX_INTERIOR || cell->page_type == INDEX_LEAF)
                    key = cell->fields[0].value.integer;

                if (key >= end_key) {
                    printf("found end of range\n");
                    break;
                }
                print_cell(cell);
                if (!query_step(iter)) {
                    printf("step returned false\n");
                    iter = NULL;
                    break;
                }
            }
            if (iter != NULL)
                free_query_iterator(iter);
        }
    }

    int close_ret = close(fd);
    assert(close_ret >= 0);

    return 0;
}
