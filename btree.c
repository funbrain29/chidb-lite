#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "btree.h"
#define UNUSED(x) (void)(x)

static const char *PAGE_TYPE_STRING[] = {
    "", "", "index_interior", "", "",
    "table_interior", "", "", "", "",
    "index_leaf", "", "", "table_leaf"
};

struct btree_page *unpack_btree_page(uint8_t *raw, uint32_t size, uint32_t page_number) {
    struct serializer ss = { .base = raw, .size = size, .cursor = 0 };
    struct btree_page *bpage = must_malloc(size);

    bpage->number = page_number;

    bpage->type = unpack_uint8(&ss);
    unpack_uint16(&ss); // unused
    bpage->cell_count = unpack_uint16(&ss);
    unpack_uint16(&ss); // unused
    unpack_uint8(&ss); // unused

    enum btree_page_type pagetype;
    pagetype = bpage->type;

    if (pagetype == btree_index_interior || pagetype == btree_table_interior) {
        bpage->right_child = unpack_uint32(&ss);
    }

    // sizeof tells malloc how much to allocate for the type of object given
    uint16_t n = bpage->cell_count;

    bpage->cells = must_malloc(sizeof(struct btree_cell) * n);

    //loop for getting cells from cell pointers
    for (uint16_t i = 0; i < bpage->cell_count; i++ ) {
        struct btree_cell cell;
        uint32_t celloffset = unpack_uint16(&ss);
        unpack_btree_cell(pagetype,raw,size,celloffset,&cell);
        bpage->cells[i] = cell;
    }

    return bpage;
}

void free_btree_page(struct btree_page *bpage) {
    printf("Freeing page memory\n");
    for (uint16_t i = 0; i < bpage->cell_count; i++ ) {
        free_btree_cell(&(bpage->cells[i]));
    }
    free(bpage->cells);
    free(bpage);
}

void free_btree_cell(struct btree_cell *cell) {
    printf("Freeing cell memory\n");
    for (uint16_t i = 0; i < cell->field_count; i++ ) {
        switch (cell->fields[i].type)
        {
        case btree_text:
            free(cell->fields[i].value.text.string);
            break;
        case btree_blob:
            free(cell->fields[i].value.blob.data);
            break;
        default:
            break;
        }
    }
    free(cell->fields);
}

void unpack_btree_cell(enum btree_page_type type, uint8_t *raw, uint32_t size, uint32_t cursor, struct btree_cell *cell) {
    printf("\ncellpointer cursor  : %d\n", cursor);
    printf("page size           : %d\n", size);
    printf("page type name      : %s\n",PAGE_TYPE_STRING[type]);
    cell->field_count = (uint16_t)0;

    // construct the main serializer
    struct serializer ss = { .base = raw, .size = size, .cursor = cursor };
    // check for page types
    if (type == btree_index_interior) {
        cell->left_child = unpack_uint32(&ss);
    } else if (type == btree_table_interior) {
        cell->left_child = unpack_uint32(&ss);
        cell->key = unpack_varint(&ss);
        return;
    }

    // get number of fields in cell
    uint64_t header_length = unpack_varint(&ss);
    printf("header length       : %ld\n", header_length);

    // check if need to save rowid key
    if (type == btree_table_leaf) {
        cell->key = unpack_varint(&ss);
    }

    uint64_t payload_length = unpack_varint(&ss);
    cell->field_count = unpack_btree_cell_helper(raw, size, cursor, payload_length);
    printf("cell field count    : %d\n\n", cell->field_count);

    // allocate array of field values in cell
    cell->fields = must_malloc(sizeof(struct btree_field) * cell->field_count);

    // create a second serializer at the beginning of the payload
    struct serializer ss2 = { .base = raw, .size = size, .cursor = ss.cursor + cell->field_count };

    //for loop field_count times
    for (uint16_t i = 0; i < cell->field_count; i++) {
        uint64_t serial_type = unpack_varint(&ss);
        printf("\tcursor value: %d\n", ss2.cursor);
        printf("\tserial type : %ld\n", serial_type);

        // determine the type of field by the value from the first serializer
        // and set the cell value based on the type and value from second serializer

        // NULL
        if (serial_type == 0) {
            cell->fields[i].type = btree_null;
        // INT
        } else if ((serial_type > 0 && serial_type <= 6) || serial_type == 8 || serial_type == 9) {
            cell->fields[i].type = btree_integer;
            switch (serial_type) {
            case 1:
                cell->fields[i].value.integer = unpack_uint8(&ss2);
                break;
            case 2:
                cell->fields[i].value.integer = unpack_uint16(&ss2);
                break;
            case 3:
                cell->fields[i].value.integer = unpack_uint24(&ss2);
                break;
            case 4:
                cell->fields[i].value.integer = unpack_uint32(&ss2);
                break;
            case 5:
                cell->fields[i].value.integer = unpack_uint48(&ss2);
                break;
            case 6:
                cell->fields[i].value.integer = unpack_uint64(&ss2);
                break;
            case 8:
                cell->fields[i].value.integer = 0;
                break;
            case 9:
                cell->fields[i].value.integer = 1;
                break;
            default:
                assert((serial_type > 0 && serial_type <= 6) || serial_type == 8 || serial_type == 9);
            }
            printf("\tfield value : %ld\n", cell->fields->value.integer);
        // FLOAT
        } else if (serial_type == 7 ) {
            cell->fields[i].type = btree_float;
            cell->fields[i].value.floating = unpack_double(&ss2);
            printf("\tfield value : %f\n", cell->fields->value.floating);
        // BLOB
        } else if (serial_type >= 12 && serial_type % 2 == 0) {
            cell->fields[i].type = btree_blob;
            // first get the length
            uint32_t length = (serial_type - 12) / 2;
            cell->fields[i].value.blob.length = length;
            // then malloc space for length many items
            cell->fields[i].value.blob.data = must_malloc(sizeof(uint8_t) * length);

            // unpack ss2 'length' times
            for (uint32_t j = 0; j < length; j++) {
                cell->fields[i].value.blob.data[j] = unpack_uint8(&ss2);
            }
        // STR
        } else if (serial_type >= 13 && serial_type % 2 == 1) {
            cell->fields[i].type = btree_text;
            // First get the length
            uint32_t length = (serial_type - 13) / 2;
            cell->fields[i].value.text.length = length;
            // then malloc space for length many items
            cell->fields[i].value.text.string = must_malloc(sizeof(uint8_t) * length + 1); // 1 extra byte for ending char

            // unpack ss2 'length' times
            for (uint32_t j = 0; j < length; j++) {
                cell->fields[i].value.text.string[j] = unpack_uint8(&ss2);
            }
            // set ending char of the string
            cell->fields[i].value.text.string[length] = 0;
            printf("\tfield value : %s\n", cell->fields[i].value.text.string);
        // ERR
        } else {
            assert(serial_type != 10 && serial_type != 11);
        }
    printf("\n");
    }
}

// returns the number of fields in a btree cell
uint16_t unpack_btree_cell_helper(uint8_t *raw, uint32_t size, uint32_t cursor, uint64_t header_length) {
    struct serializer sh = { .base = raw, .size = size, .cursor = cursor };
    uint16_t total = 0;

    while (total + cursor < header_length + cursor ) {
        unpack_varint(&sh);
        total++;
    }

    return total - 1;
}

void print_btree_page(struct btree_page *bpage) {
    printf("page number         : %d\n", bpage->number);
    printf("page type           : %d\n", bpage->type);
    printf("page type name      : %s\n",PAGE_TYPE_STRING[bpage->type]);
    printf("page cell_count     : %d\n", bpage->cell_count);

    enum btree_page_type pagetype;
    pagetype = bpage->type;

    if (pagetype == btree_index_interior || pagetype == btree_table_interior) {
        printf("page right_child    : %d\n", bpage->right_child);
    }
}

void unpack_file_header(uint8_t *raw, uint32_t size, struct file_header *head) {
    struct serializer ss = { .base = raw, .size = size, .cursor = 0 };
    unpack_raw_string(&ss, (uint8_t *) &head->header_string, 16);

    // get the order to do these from the struct file_header
    head->page_size = unpack_uint16(&ss);
    head->file_format_write_version = unpack_uint8(&ss);
    head->file_format_read_version = unpack_uint8(&ss);
    head->bytes_reserved_at_end_of_each_page = unpack_uint8(&ss);
    head->max_embedded_payload_fraction = unpack_uint8(&ss);
    head->min_embedded_payload_fraction = unpack_uint8(&ss);
    head->min_leaf_payload_fraction = unpack_uint8(&ss);
    head->file_change_counter = unpack_uint32(&ss);
    head->file_size_in_pages = unpack_uint32(&ss);

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

    /* ss is automatically deallocated as part of the stack frame */
}

void print_file_header(struct file_header *head) {
    printf("header string                     : %s\n", head->header_string);

    printf("page size                         : %d\n", head->page_size);
    printf("file format write version         : %d\n", head->file_format_write_version);
    printf("file format read version          : %d\n", head->file_format_read_version);
    printf("bytes reserved at end of each page: %d\n", head->bytes_reserved_at_end_of_each_page);
    printf("max embedded payload fraction     : %d\n", head->max_embedded_payload_fraction);
    printf("min embedded payload fraction     : %d\n", head->min_embedded_payload_fraction);
    printf("min leaf payload fraction         : %d\n", head->min_leaf_payload_fraction);
    printf("file change counter               : %d\n", head->file_change_counter);
    printf("file size in pages                : %d\n", head->file_size_in_pages);

    printf("first freelist page               : %d\n", head->first_freelist_page);
    printf("number of freelist pages          : %d\n", head->number_of_freelist_pages);
    printf("schema version cookie             : %d\n", head->schema_version_cookie);
    printf("schema format number              : %d\n", head->schema_format_number);

    printf("page cache size                   : %d\n", head->page_cache_size);
    printf("vacuum page number                : %d\n", head->vacuum_page_number);
    printf("text encoding                     : %d\n", head->text_encoding);
    printf("user version                      : %d\n", head->user_version);

    printf("auto vacuum mode                  : %d\n", head->auto_vacuum_mode);
    printf("application id                    : %d\n", head->application_id);
    printf("version valid for                 : %d\n", head->version_valid_for);
    printf("sqlite version number             : %d\n", head->sqlite_version_number);
}