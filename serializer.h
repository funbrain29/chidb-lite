#pragma once

#include <inttypes.h>
#include <stddef.h>

struct serializer {
    uint8_t *base;
    uint32_t size;
    uint32_t cursor;
};

void *must_malloc(size_t size);
void skip(struct serializer *ss, uint32_t bytes);
void skip_to(struct serializer *ss, uint32_t new_cursor);
uint8_t unpack_uint8(struct serializer *ss);
int8_t unpack_int8(struct serializer *ss);
uint16_t unpack_uint16(struct serializer *ss);
int16_t unpack_int16(struct serializer *ss);
uint32_t unpack_uint24(struct serializer *ss);
int32_t unpack_int24(struct serializer *ss);
uint32_t unpack_uint32(struct serializer *ss);
int32_t unpack_int32(struct serializer *ss);
uint64_t unpack_uint48(struct serializer *ss);
int64_t unpack_int48(struct serializer *ss);
uint64_t unpack_uint64(struct serializer *ss);
int64_t unpack_int64(struct serializer *ss);
double unpack_double(struct serializer *ss);
int64_t unpack_varint(struct serializer *ss);
void unpack_raw_string(struct serializer *ss, uint8_t *buffer, uint32_t length);
uint8_t *unpack_raw_string_malloc(struct serializer *ss, uint32_t length);
uint8_t *unpack_blob_malloc(struct serializer *ss, uint32_t length);
