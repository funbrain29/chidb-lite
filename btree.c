#include "btree.h"


struct btree_page *unpack_btree_page(uint8_t *raw, uint32_t size, uint32_t page_number) {
    struct serializer ss = { .base = raw, .size = size, .cursor = 0 };
    struct btree_page *bpage = must_malloc(size);
    bpage->number = page_number;
    bpage->type = unpack_uint8(&ss);
    printf("page number         : %d\n", bpage->number);
    printf("page type           : %d\n", bpage->type);
    return bpage;
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