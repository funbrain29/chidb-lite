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

int first_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root);
int second_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root);
int third_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root, uint32_t crew_titleid_index_root);

int main() {
    int querynumber = 1;
    int pagesizenumber = 1;

    int returnValue = 0;
    int fd;
    struct file_header head;
    uint32_t people_root;
    uint32_t titles_root;
    uint32_t crew_root;
    uint32_t crew_titleid_index_root;

    // if you are loading from a file other than test.db, change this
    if (pagesizenumber == 1) {
        fd = open("imdb-4096.db", O_RDONLY);
        people_root = 2;
        titles_root = 72;
        crew_root = 118;
        crew_titleid_index_root = 287;
        assert(fd >= 0);
    }
    if (pagesizenumber == 2) {
        fd = open("imdb-512.db", O_RDONLY);
        people_root = 2;
        titles_root = 579;
        crew_root = 979;
        crew_titleid_index_root = 2393;
        assert(fd >= 0);
    }
    uint8_t *raw = must_malloc(FILE_HEADER_SIZE);
    ssize_t ret = read(fd, raw, FILE_HEADER_SIZE);
    assert(ret == FILE_HEADER_SIZE);

    /* read and unpack the header */
    unpack_file_header(raw, FILE_HEADER_SIZE, &head);
    free(raw);

    // Query functions go here:
    if (querynumber == 1) {
        printf("Performing first query:\n");
        returnValue = first_query(head,fd,people_root,titles_root,crew_root);
        assert(returnValue == 0);
    } else if (querynumber == 2) {
        printf("\nPerforming second query:\n");
        returnValue = second_query(head,fd,people_root,titles_root,crew_root);
        assert(returnValue == 0);
    } else if (querynumber == 3) {
        printf("\nPerforming third query:\n");
        returnValue = third_query(head,fd,people_root,titles_root,crew_root,crew_titleid_index_root);
        assert(returnValue == 0);
    }

    // close file and return
    int close_ret = close(fd);
    assert(close_ret >= 0);

    return returnValue;
}

// Scan the titles table to find the title_id of ‘Inception’, then scan the crew table and for each record with matching title_id lookup the record in people and print the result.
int first_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root) {
    // set count to track seeks
    int count = 0;

    struct query_iterator *title_iter = query_seek(&head, fd, titles_root, 1);
    if (title_iter == NULL)  {
        // note: when query_seek returns NULL, it means the key was not found
        // and there is no memory allocation to worry about
        printf("not found\n");
        return 0;
    }
    int64_t title_key = 0;
    do {
        struct cell *cell = query_get(title_iter);
        if (!strcmp((const char *) cell->fields[2].value.text.string, "Inception")) {
            title_key = cell->key;
        }
        count++;
    } while (query_step(title_iter));


    struct query_iterator *crew_iter = query_seek(&head, fd, crew_root, 1);
    if (crew_iter == NULL)  {
        printf("not found\n");
        return 0;
    }
    struct cell* crew_cells[10];
    int i = 0;
    do {
        struct cell *cell = query_get(crew_iter);
        if (title_key == cell->fields[0].value.integer) {
            crew_cells[i] = cell;
            i++;
        }
        count++;
    } while (query_step(crew_iter));


for (int j = 0; j < i;j++) {
    struct query_iterator *people_iter = query_seek(&head, fd, people_root, 1);
    if (people_iter == NULL)  {
        printf("not found\n");
        return 0;
    }

    do {
        struct cell *cell = query_get(people_iter);
        if (crew_cells[j]->fields[1].value.integer == cell->key) { // match person id
            print_cell(cell);
        }
        count++;
    } while (query_step(people_iter));
}

    // note: when query_step returns false, it means there are no more records
    // left and all memory from the iterator has been freed already.
    // If you finish using an iterator before query_step returns false, you must
    // call free_query_iterator on it to clean up.

    printf("found %d entries\n", count);

    // report on number of steps, page loads, etc.
    report();

    return 0;
}

// Scan the crew table, and for each record look up the corresponding title entry. If it’s a match, look up the corresponding person entry and print it.

int second_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root) {
    // set count to track seeks
    int count = 0;

    struct query_iterator *crew_iter = query_seek(&head, fd, crew_root, 1);
    if (crew_iter == NULL)  {
        printf("not found\n");
        return 0;
    }
    do { // for every item in the crew table
        struct cell *crewcell = query_get(crew_iter);

        struct query_iterator *titles_iter = query_seek(&head, fd, titles_root, crewcell->fields[0].value.integer);
        if (titles_iter == NULL)  {
            printf("not found\n");
            return 0;
        }

        do { // for every item in the title table
            struct cell *titlecell = query_get(titles_iter);
            if (crewcell->fields[0].value.integer == titlecell->key) {
                if (!strcmp((const char *) titlecell->fields[2].value.text.string, "Inception")) {

                    struct query_iterator *people_iter = query_seek(&head, fd, people_root, 1);
                    if (people_iter == NULL)  {
                        printf("not found\n");
                        return 0;
                    }

                    do { // for every item in the people table
                        struct cell *peoplecell = query_get(people_iter);
                        if (peoplecell->key == crewcell->fields[1].value.integer) {
                            print_cell(peoplecell);
                        }
                    } while (query_step(people_iter));
                } else {
                    free_query_iterator(titles_iter);
                    break;
                }
            }
            count++;
        } while (query_step(titles_iter));
        count++;
    } while (query_step(crew_iter));

    printf("found %d entries\n", count);

    report();

    return 0;
}

// Create an index:
// CREATE INDEX crew_title_id ON crew (title_id);
// Search the titles table to find the title_id, use the index to find matching crew entries, and for each one lookup the matching person record and print out info.
int third_query(struct file_header head, int fd, uint32_t people_root, uint32_t titles_root, uint32_t crew_root, uint32_t crew_titleid_index_root) {
    int count = 0;

    struct query_iterator *title_iter = query_seek(&head, fd, titles_root, 1);
    if (title_iter == NULL)  {
        printf("not found\n");
        return 0;
    }
    int64_t title_key = 0;
    do {
        struct cell *cell = query_get(title_iter);
        if (!strcmp((const char *) cell->fields[2].value.text.string, "Inception")) {
            title_key = cell->key;
        }
        count++;
    } while (query_step(title_iter));


    struct query_iterator *index_iter = query_seek(&head, fd, crew_titleid_index_root, title_key);
    if (index_iter == NULL)  {
        printf("not found\n");
        return 0;
    }
    do {
        struct cell *indexcell = query_get(index_iter);
        if (indexcell->fields[0].value.integer == title_key) {
            int crew_key = indexcell->fields[1].value.integer;

            struct query_iterator *crew_iter = query_seek(&head, fd, crew_root, crew_key);
            if (index_iter == NULL)  {
                printf("not found\n");
                return 0;
            }

            do {
                struct cell *crewcell = query_get(crew_iter);
                if (crewcell->key == crew_key) {
                    int people_key = crewcell->fields[1].value.integer;

                    struct query_iterator *people_iter = query_seek(&head, fd, people_root, people_key);
                    if (index_iter == NULL)  {
                        printf("not found\n");
                        return 0;
                    }
                    do {
                        struct cell *peoplecell = query_get(people_iter);
                        if (peoplecell->key == people_key) {
                            print_cell(peoplecell);
                        }
                        count++;
                    } while (query_step(people_iter));

                } else {
                    free_query_iterator(crew_iter);
                    break;
                }

                count++;
            } while (query_step(crew_iter));

        } else {
            free_query_iterator(index_iter);
            break;
        }
        count++;
    } while (query_step(index_iter));

    printf("found %d entries\n", count);

    report();

    return 0;
}