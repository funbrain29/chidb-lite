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

bool if_zone(int64_t zone);
int first_version(struct file_header head, int fd, uint32_t rooms_root);
int second_version(struct file_header head, int fd, uint32_t rooms_root, uint32_t rooms_zones_index);

int main() {
    int querynumber = 2;

    int returnValue = 0;
    int fd;
    struct file_header head;

    fd = open("mud.db", O_RDONLY);
    int rooms_root = 3;
    int rooms_zones_index = 212;
    assert(fd >= 0);

    uint8_t *raw = must_malloc(FILE_HEADER_SIZE);
    ssize_t ret = read(fd, raw, FILE_HEADER_SIZE);
    assert(ret == FILE_HEADER_SIZE);

    /* read and unpack the header */
    unpack_file_header(raw, FILE_HEADER_SIZE, &head);
    free(raw);

    // Query functions go here:
    if (querynumber == 1) {
        printf("Performing first query:\n");
        returnValue = first_version(head,fd,rooms_root);
        printf("\nFirst query finished, closing program...\n");
        assert(returnValue == 0);
    } else if (querynumber == 2) {
        printf("\nPerforming second query:\n");
        returnValue = second_version(head,fd,rooms_root,rooms_zones_index);
        printf("\nSecond query finished, closing program...\n");
        assert(returnValue == 0);
    }

    // close file and return
    int close_ret = close(fd);
    assert(close_ret >= 0);

    return returnValue;
}

bool if_zone(int64_t zone) {
    switch (zone) {
        case 11:
        case 12:
        case 47:
        case 50:
        case 51:
        case 52:
        case 62:
        case 65:
        case 30:
        case 60:
        case 70:
        case 79:
        return true;
        default:
        return false;
    }
}


int first_version(struct file_header head, int fd, uint32_t rooms_root) {
    // set count to track seeks
    int count = 0;
    int printed = 0;
    const char *cave = "cave";
    char *current;

    struct query_iterator *rooms_iter = query_seek(&head, fd, rooms_root, 1);
    if (rooms_iter == NULL)  {
        // note: when query_seek returns NULL, it means the key was not found
        // and there is no memory allocation to worry about
        printf("not found\n");
        return 0;
    }
    do {
        struct cell *cell = query_get(rooms_iter);
        if (if_zone(cell->fields[1].value.integer)) {
            current = strstr((const char *)cell->fields[3].value.text.string, cave);
            if (current) {
                printf("%s\n",cell->fields[3].value.text.string);
                printed++;
            }
        }
        count++;
    } while (query_step(rooms_iter));

    printf("found %d entries\n", count);
    printf("printed %d entries\n", printed);
    report();

    return 0;
}

int second_version(struct file_header head, int fd, uint32_t rooms_root, uint32_t rooms_zones_index) {

    // set count to track seeks
    int printed = 0;
    int count = 0;
    const char *cave = "cave";
    char *current;
    int zones[12] = {11,12,47,50,51,52,62,65,30,60,70,79};

    for (int i = 0; i < 12; i++) {
        struct query_iterator *zones_iter = query_seek(&head, fd, rooms_zones_index, zones[i]);
        if (zones_iter == NULL)  {
            printf("not found\n");
            return 0;
        }
        do {
            struct cell *index_cell = query_get(zones_iter);
            // break if the next cell is not from the current zone
            if (index_cell->fields[0].value.integer != zones[i]) {
                free_query_iterator(zones_iter);
                break;
            }
            // start new iter at the room id found by the index
            struct query_iterator *rooms_iter = query_seek(&head, fd, rooms_root, index_cell->fields[1].value.integer);
            if (rooms_iter == NULL)  {
                printf("not found\n");
                return 0;
            }
            do {
                struct cell *rooms_cell = query_get(rooms_iter);
                current = strstr((const char *)rooms_cell->fields[3].value.text.string, cave);
                if (current) {
                    printf("%s\n",rooms_cell->fields[3].value.text.string);
                    printed++;
                }
                free_query_iterator(rooms_iter);
                count++;
                break;
            } while (query_step(rooms_iter));
            count++;
        } while (query_step(zones_iter));
    }

    printf("found %d entries\n", count);
    printf("printed %d entries\n", printed);
    report();

    return 0;
}
