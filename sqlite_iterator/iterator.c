#include <assert.h>
#include "btree.h"
#include "serializer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

bool chatty = false;
int page_reads = 0;
int seeks = 0;
int steps = 0;
int internal_steps = 0;

void report(void) {
    printf("page reads: %d, iterator seeks: %d, iterator steps: %d, internal steps: %d\n",
        page_reads, seeks, steps, internal_steps);
}

void free_query_iterator(struct query_iterator *iter) {
    assert(iter != NULL);
    struct query_iterator_page *current = iter->head;
    while (current != NULL) {
        free_page(current->page);
        struct query_iterator_page *old = current;
        current = current->parent;
        free(old);
    }
    free(iter);
}

struct page *get_page(struct file_header *header, int fd, uint32_t page_number) {
    page_reads++;
    if (chatty) printf("loading page %d\n", page_number);
    uint8_t *raw = must_malloc(header->page_size);
    off_t lseek_ret = lseek(fd, (page_number - 1) * header->page_size, SEEK_SET);
    assert(lseek_ret == (page_number - 1) * header->page_size);
    ssize_t ret = read(fd, raw, header->page_size);
    assert(ret == header->page_size);

    struct page *page = unpack_page(raw, header->page_size, page_number);
    free(raw);

    return page;
}

struct query_iterator *query_seek(struct file_header *header, int fd, uint32_t root, int64_t key) {
    seeks++;

    struct query_iterator *iter = must_malloc(sizeof(struct query_iterator));
    iter->header = header;
    iter->fd = fd;
    iter->head = NULL;
    for (;;) {
        struct query_iterator_page *current = must_malloc(sizeof(struct query_iterator_page));
        current->page = get_page(header, fd, root);
        current->index = 0;
        current->parent = iter->head;
        current->index_returned = false;
        iter->head = current;
        int64_t cell_key = 0;

        switch (current->page->type) {
        case TABLE_LEAF:
        case INDEX_LEAF:
            for (current->index = 0; current->index < current->page->cell_count; current->index++) {
                internal_steps++;

                struct cell *cell = &current->page->cells[current->index];
                if (current->page->type == TABLE_LEAF) {
                    cell_key = cell->key;
                } else {
                    assert(cell->field_count == 2);
                    assert(cell->fields[0].type == FIELD_INTEGER);
                    assert(cell->fields[1].type == FIELD_INTEGER);
                    cell_key = cell->fields[0].value.integer;
                }
                if (cell_key >= key) {
                    if (chatty) printf("seek found starting cell at index %u of %u\n", current->index, current->page->cell_count - 1);
                    return iter;
                }
            }

            if (current->page->type == TABLE_LEAF) {
                // not found: clean up and return NULL
                if (chatty) printf("seek did not find a matching entry after scanning entire table leaf, giving up\n");
                free_query_iterator(iter);
                return NULL;
            } else {
                // not found: the interior node that led us here might have been the
                // first hit, so try stepping once
                if (chatty) printf("seek did not find a matching entry after scanning index leaf, trying to step once\n");
                if (query_step(iter)) {
                    if (chatty) printf("seek found a valid entry by stepping\n");
                    return iter;
                } else {
                    // step reached the end and cleaned everything up
                    if (chatty) printf("seek did not find an entry by stepping, giving up\n");
                    return NULL;
                }
            }

        case TABLE_INTERIOR:
        case INDEX_INTERIOR:
            // following the right child is the last resort
            root = current->page->right_child;
            for (current->index = 0; current->index < current->page->cell_count; current->index++) {
                internal_steps++;

                struct cell *cell = &current->page->cells[current->index];
                if (current->page->type == TABLE_INTERIOR) {
                    cell_key = cell->key;
                } else {
                    assert(cell->field_count == 2);
                    assert(cell->fields[0].type == FIELD_INTEGER);
                    assert(cell->fields[1].type == FIELD_INTEGER);
                    cell_key = cell->fields[0].value.integer;
                }
                if (cell_key >= key) {
                    if (chatty) printf("seek following left child of cell %u of %u\n", current->index, current->page->cell_count - 1);
                    root = cell->left_child;
                    break;
                }
            }

            // break to outer loop, which will follow the child link
            if (root == current->page->right_child)
                if (chatty) printf("seek following right child\n");
            break;

        default:
            assert(false);
        }
    }

    return NULL;
}

struct cell *query_get(struct query_iterator *iter) {
    assert(iter != NULL);

    return &iter->head->page->cells[iter->head->index];
}

bool query_step(struct query_iterator *iter) {
    assert(iter != NULL);
    steps++;

    struct query_iterator_page *current = iter->head;
    struct query_iterator_page *old;
    for (;;) {
        if (current == NULL) {
            free(iter);
            if (chatty) printf("step backtracked from the root node, nothing more to step over\n");
            return false;
        }

        switch (current->page->type) {
        case TABLE_LEAF:
        case INDEX_LEAF:
            internal_steps++;
            current->index++;
            if (current->index < current->page->cell_count) {
                return true;
            }

            // walk back up to the interior page above this leaf
            if (chatty) printf("step is backtracking from a leaf node\n");
            free_page(current->page);
            old = current;
            current = current->parent;
            free(old);
            iter->head = current;
            break;

        case INDEX_INTERIOR:
            if (current->index < current->page->cell_count && !current->index_returned) {
                internal_steps++;
                if (chatty) printf("step is returning an index interior node at index %u of %u\n", current->index, current->page->cell_count - 1);
                current->index_returned = true;
                return true;
            }

            // fallthrough
        case TABLE_INTERIOR:
            internal_steps++;
            current->index++;
            if (current->index <= current->page->cell_count) {
                uint32_t link;
                if (current->index < current->page->cell_count) {
                    if (chatty) printf("step following left child of cell %u of %u\n", current->index, current->page->cell_count - 1);
                    link = current->page->cells[current->index].left_child;
                    current->index_returned = false;
                } else {
                    if (chatty) printf("step following right child\n");
                    link = current->page->right_child;
                }

                // walk down to next child
                struct query_iterator_page *newpage = must_malloc(sizeof(struct query_iterator_page));
                newpage->page = get_page(iter->header, iter->fd, link);
                newpage->index = -1;
                newpage->parent = current;
                newpage->index_returned = true;
                current = newpage;
                iter->head = current;
                break;
            }

            // walk up to the interior page above this one
            if (chatty) printf("seek is backtracking from an interior node\n");
            free_page(current->page);
            old = current;
            current = current->parent;
            free(old);
            iter->head = current;
            break;

        default:
            assert(false);
        }
    }

    return false;
}
