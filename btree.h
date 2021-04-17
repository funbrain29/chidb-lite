#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "serializer.h"

#define FILE_HEADER_SIZE 100

enum btree_page_type {
    btree_index_interior = 2,
    btree_table_interior = 5,
    btree_index_leaf = 10,
    btree_table_leaf = 13
};

struct file_header {
    uint8_t header_string[16];

    uint16_t page_size;
    uint8_t file_format_write_version;
    uint8_t file_format_read_version;
    uint8_t bytes_reserved_at_end_of_each_page;
    uint8_t max_embedded_payload_fraction;
    uint8_t min_embedded_payload_fraction;
    uint8_t min_leaf_payload_fraction;
    uint32_t file_change_counter;
    uint32_t file_size_in_pages;

    uint32_t first_freelist_page;
    uint32_t number_of_freelist_pages;
    uint32_t schema_version_cookie;
    uint32_t schema_format_number;

    uint32_t page_cache_size;
    uint32_t vacuum_page_number;
    uint32_t text_encoding;
    uint32_t user_version;

    uint32_t auto_vacuum_mode;
    uint32_t application_id;
    uint32_t version_valid_for;
    uint32_t sqlite_version_number;

    uint32_t unused1;
    uint32_t unused2;
    uint32_t unused3;
    uint32_t unused4;

    uint32_t unused5;
};

struct btree_page {
    uint32_t number;
    enum btree_page_type type;
    uint16_t cell_count;
    uint32_t right_child;
    struct btree_cell *cells;
};

enum btree_field_type {
    btree_null,
    btree_integer,
    btree_float,
    btree_text,
    btree_blob
};
struct btree_cell {
    uint32_t left_child;
    int64_t key;
    uint16_t field_count;
    struct btree_field *fields;
};

struct btree_field {
    enum btree_field_type type;
    union {
        int64_t integer;
        double floating;
        struct {
            uint32_t length;
            uint8_t *string;
        } text;
        struct {
            uint32_t length;
            uint8_t *data;
        } blob;
    } value;
};

void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head);
void unpack_btree_cell(enum btree_page_type type, uint8_t *raw, uint32_t size, uint32_t cursor, struct btree_cell *cell);
struct btree_page *unpack_btree_page(uint8_t *raw, uint32_t size, uint32_t page_number);
void free_btree_cell(struct btree_cell *cell);
void free_btree_page(struct btree_page *page);
uint16_t unpack_btree_cell_helper(uint8_t *raw, uint32_t size, uint32_t cursor, uint64_t header_length);