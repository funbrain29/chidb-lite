#include <assert.h>
#include "btree.h"
#include "serializer.h"
#include <stdlib.h>

void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head) {
    struct serializer ss = { .base = raw, .size = size, .cursor = 0 };

    unpack_raw_string(&ss, (uint8_t *) &head->header_string, 16);
    assert(head->header_string[15] == 0);

    head->page_size = unpack_uint16(&ss);
    head->file_format_write_version = unpack_uint8(&ss);
    head->file_format_read_version = unpack_uint8(&ss);
    head->bytes_reserved_at_end_of_each_page = unpack_uint8(&ss);
    head->max_embedded_payload_fraction = unpack_uint8(&ss);
    head->min_embedded_payload_fraction = unpack_uint8(&ss);
    head->min_leaf_payload_fraction = unpack_uint8(&ss);
    head->file_change_counter = unpack_uint8(&ss);
    head->file_size_in_pages = unpack_uint8(&ss);

    head->first_freelist_page = unpack_uint32(&ss);
    head->number_of_freelist_pages = unpack_uint32(&ss);
    head->schema_version_cookie = unpack_uint32(&ss);
    head->schema_format_number = unpack_uint32(&ss);

    head->page_cache_size = unpack_uint32(&ss);
    head->vacuum_page_number = unpack_uint32(&ss);
    head->text_encoding = unpack_uint32(&ss);
    head->user_version = unpack_uint32(&ss);

    head->auto_vacuum_mode = unpack_uint32(&ss);
    head->application_id = unpack_uint32(&ss);
    head->version_valid_for = unpack_uint32(&ss);
    head->sqlite_version_number = unpack_uint32(&ss);

    head->unused1 = unpack_uint32(&ss);
    head->unused2 = unpack_uint32(&ss);
    head->unused3 = unpack_uint32(&ss);
    head->unused4 = unpack_uint32(&ss);

    head->unused5 = unpack_uint32(&ss);
}

struct page *unpack_page(uint8_t *raw, uint32_t size, uint32_t page_number) {
    struct serializer ss = { .base = raw, .size = size, .cursor = page_number == 1 ? FILE_HEADER_SIZE : 0 };
    struct page *page = (struct page *) must_malloc(sizeof(struct page));
    page->number = page_number;

    /* parse the header */
    page->type = (enum page_type) unpack_uint8(&ss);
    assert(page->type == INDEX_INTERIOR ||
           page->type == TABLE_INTERIOR ||
           page->type == INDEX_LEAF ||
           page->type == TABLE_LEAF);
    unpack_uint16(&ss); /* free offset */
    page->cell_count = unpack_uint16(&ss);
    unpack_uint16(&ss); /* cell offset */
    unpack_uint8(&ss); /* number of fragmented free bytes */
    page->right_child = 0;
    if (page->type == INDEX_INTERIOR || page->type == TABLE_INTERIOR) {
        page->right_child = unpack_uint32(&ss);
    }

    page->cells = (struct cell *) must_malloc(sizeof(struct cell) * page->cell_count);

    /* parse the cells */
    for (uint16_t i = 0; i < page->cell_count; i++) {
        uint16_t cursor = unpack_uint16(&ss);
        unpack_cell(page->type, raw, size, cursor, &page->cells[i]);
    }

    return page;
}

uint32_t count_cell_fields(uint8_t *raw, uint32_t size) {
    struct serializer ss = { .base = raw, .size = size, .cursor = 0 };
    uint32_t count = 0;
    while (ss.cursor < size) {
        unpack_varint(&ss);
        count++;
    }
    return count;
}

void unpack_cell(enum page_type type, uint8_t *raw, uint32_t size, uint32_t cursor, struct cell *cell) {
    struct serializer ss = { .base = raw, .size = size, .cursor = cursor };
    cell->page_type = type;

    /* left child link (interior nodes only) */
    cell->left_child = 0;
    if (type == TABLE_INTERIOR || type == INDEX_INTERIOR)
        cell->left_child = unpack_uint32(&ss);

    /* bytes of payload (all but table interior nodes) */
    uint32_t record_size = 0;
    if (type != TABLE_INTERIOR) {
        record_size = (uint32_t) unpack_varint(&ss);
    }

    /* key/rowid (table nodes only) */
    cell->key = 0;
    if (type == TABLE_INTERIOR || type == TABLE_LEAF)
        cell->key = unpack_varint(&ss);

    /* payload (all but table interior nodes) */
    cell->field_count = 0;
    cell->fields = 0;
    if (type != TABLE_INTERIOR) {
        /* do not read beyond the end of the payload */
        if (ss.size > ss.cursor + record_size)
            ss.size = ss.cursor + record_size;

        struct serializer vv = ss;
        uint32_t field_header_size = unpack_varint(&ss);
        vv.cursor += field_header_size;
        cell->field_count = count_cell_fields(ss.base + ss.cursor, vv.cursor - ss.cursor);
        cell->fields = (struct field *) must_malloc(sizeof(struct field) * cell->field_count);

        /* decode the fields */
        for (uint16_t i = 0; i < cell->field_count; i++) {
            int64_t type_code = unpack_varint(&ss);
            switch (type_code) {
            case 0:
                cell->fields[i].type = FIELD_NULL;
                break;

            case 1:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int8(&vv);
                break;

            case 2:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int16(&vv);
                break;

            case 3:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int24(&vv);
                break;

            case 4:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int32(&vv);
                break;

            case 5:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int48(&vv);
                break;

            case 6:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = unpack_int64(&vv);
                break;

            case 7:
                cell->fields[i].type = FIELD_FLOAT;
                cell->fields[i].value.floating = unpack_double(&vv);
                break;

            case 8:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = 0;
                break;

            case 9:
                cell->fields[i].type = FIELD_INTEGER;
                cell->fields[i].value.integer = 1;
                break;

            case 10:
            case 11:
                assert(type_code != 10 && type_code != 11);

            default:
                if ((type_code & 1) == 0) {
                    /* blob */
                    cell->fields[i].type = FIELD_BLOB;
                    uint32_t length = cell->fields[i].value.blob.length = (type_code - 12) / 2;
                    cell->fields[i].value.blob.data = unpack_blob_malloc(&vv, length);
                } else {
                    /* text */
                    cell->fields[i].type = FIELD_TEXT;
                    uint32_t length = cell->fields[i].value.text.length = (type_code - 13) / 2;
                    cell->fields[i].value.text.string = unpack_raw_string_malloc(&vv, length);
                }
            }
        }
    }

    /* overflow link if applicable (all but table interior nodes) */
    /* we ignore this possibility */
}

void free_cell(struct cell *cell) {
    for (uint16_t i = 0; i < cell->field_count; i++) {
        if (cell->fields[i].type == FIELD_TEXT)
            free(cell->fields[i].value.text.string);
        else if (cell->fields[i].type == FIELD_BLOB)
            free(cell->fields[i].value.blob.data);
    }
    free(cell->fields);
}

void free_page(struct page *page) {
    if (page->cell_count > 0) {
        for (uint16_t i = 0; i < page->cell_count; i++)
            free_cell(&page->cells[i]);
        free(page->cells);
    }
    free(page);
}
