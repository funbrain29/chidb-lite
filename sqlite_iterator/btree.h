#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define FILE_HEADER_SIZE 100

enum page_type {
    INDEX_INTERIOR = 2,
    TABLE_INTERIOR = 5,
    INDEX_LEAF = 10,
    TABLE_LEAF = 13
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

struct page {
    uint32_t number;
    enum page_type type;
    uint16_t cell_count;
    uint32_t right_child;
    struct cell *cells;
};

struct cell {
    enum page_type page_type;
    uint32_t left_child;
    int64_t key;
    uint16_t field_count;
    struct field *fields;
};

enum field_type {
    FIELD_NULL,
    FIELD_INTEGER,
    FIELD_FLOAT,
    FIELD_TEXT,
    FIELD_BLOB
};

struct field {
    enum field_type type;
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

struct query_iterator_page {
    struct page *page;
    uint16_t index;
    bool index_returned;
    struct query_iterator_page *parent;
};

struct query_iterator {
    struct file_header *header;
    int fd;
    struct query_iterator_page *head;
};


void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head);
void unpack_cell(enum page_type type, uint8_t *raw, uint32_t size, uint32_t cursor, struct cell *cell);
struct page *unpack_page(uint8_t *raw, uint32_t size, uint32_t page_number);
void free_cell(struct cell *cell);
void free_page(struct page *page);

void print_file_header(struct file_header *head);
void print_page(struct page *page);
void print_cell(struct cell *cell);

void free_query_iterator(struct query_iterator *wrapper);
struct query_iterator *query_seek(struct file_header *header, int fd, uint32_t root, int64_t key);
struct cell *query_get(struct query_iterator *iter);
bool query_step(struct query_iterator *iter);
void report(void);
