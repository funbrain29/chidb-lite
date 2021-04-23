#include <assert.h>
#include "btree.h"
#include <stdbool.h>
#include <stdio.h>

void print_cell(struct cell *cell) {
    bool print_fields = true;
    if (cell->page_type == TABLE_INTERIOR) {
        printf("cell has left child %u and rowid %lu\n", cell->left_child, cell->key);
        print_fields = false;
    } else if (cell->page_type == INDEX_INTERIOR && cell->field_count == 2 && cell->fields[0].type == FIELD_INTEGER && cell->fields[1].type == FIELD_INTEGER) {
        printf("cell has left child %u and is an index mapping %lu -> %lu\n", cell->left_child, cell->fields[0].value.integer, cell->fields[1].value.integer);
        print_fields = false;
    } else if (cell->page_type == INDEX_LEAF && cell->field_count == 2 && cell->fields[0].type == FIELD_INTEGER && cell->fields[1].type == FIELD_INTEGER) {
        printf("cell is an index mapping %lu -> %lu\n", cell->fields[0].value.integer, cell->fields[1].value.integer);
        print_fields = false;
    } else if (cell->page_type == INDEX_INTERIOR) {
        printf("cell has left child %u and %u fields\n", cell->left_child, cell->field_count);
    } else {
        //printf("cell has %u fields\n", cell->field_count);
    }
    if (print_fields) {
        printf("cell (%lu: ", cell->key);
        for (uint16_t j = 0; j < cell->field_count; j++) {
            struct field *field = &cell->fields[j];
            if (j > 0) printf(", ");
            switch (field->type) {
                case FIELD_NULL:
                    printf("NULL");
                    break;
                case FIELD_INTEGER:
                    printf("%ld", field->value.integer);
                    break;
                case FIELD_FLOAT:
                    printf("%f", field->value.floating);
                    break;
                case FIELD_TEXT:
                    printf("'%s'", field->value.text.string);
                    break;
                case FIELD_BLOB:
                    printf("%d-byte blob", field->value.blob.length);
                    break;
                default:
                    assert(field->type);
            }
        }
        printf(")\n");
    }
}

void print_page(struct page *page) {
    switch (page->type) {
    case INDEX_INTERIOR:
        printf("%d is an interior index page with %d cells and right child %d\n", page->number, page->cell_count, page->right_child);
        break;
    case TABLE_INTERIOR:
        printf("%d is an interior table page with %d cells and right child %d\n", page->number, page->cell_count, page->right_child);
        break;
    case INDEX_LEAF:
        printf("%d is a leaf index page with %d cells\n", page->number, page->cell_count);
        break;
    case TABLE_LEAF:
        printf("%d is a leaf table page with %d cells\n", page->number, page->cell_count);
        break;
    }

    for (uint32_t i = 0; i < page->cell_count; i++) {
        struct cell *cell = &page->cells[i];
        print_cell(cell);
    }
}

void print_file_header(struct file_header *head) {
    printf("header string                       : %s\n", head->header_string);

    printf("page size                           : %u\n", head->page_size);
    printf("file format write version           : %u\n", head->file_format_write_version);
    printf("file format read version            : %u\n", head->file_format_read_version);
    printf("bytes reserved at end of each page  : %u\n", head->bytes_reserved_at_end_of_each_page);
    printf("max embedded payload fraction       : %u\n", head->max_embedded_payload_fraction);
    printf("min embedded payload fraction       : %u\n", head->min_embedded_payload_fraction);
    printf("min leaf payload fraction           : %u\n", head->min_leaf_payload_fraction);
    printf("file change counter                 : %u\n", head->file_change_counter);
    printf("file size in pages                  : %u\n", head->file_size_in_pages);

    printf("first freelist page                 : %u\n", head->first_freelist_page);
    printf("number of freelist pages            : %u\n", head->number_of_freelist_pages);
    printf("schema version cookie               : %u\n", head->schema_version_cookie);
    printf("schema format number                : %u\n", head->schema_format_number);

    printf("page cache size                     : %u\n", head->page_cache_size);
    printf("vacuum page number                  : %u\n", head->vacuum_page_number);
    printf("text encoding                       : %u\n", head->text_encoding);
    printf("user version                        : %u\n", head->user_version);

    printf("auto vacuum mode                    : %u\n", head->auto_vacuum_mode);
    printf("application id                      : %u\n", head->application_id);
    printf("version valid for                   : %u\n", head->version_valid_for);
    printf("sqlite version number               : %u\n", head->sqlite_version_number);
}
