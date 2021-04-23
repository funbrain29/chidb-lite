#include <assert.h>
#include "serializer.h"
#include <string.h>
#include <stdlib.h>

void *must_malloc(size_t size) {
    void *result = malloc(size);
    assert(result != NULL);
    return result;
}

void skip(struct serializer *ss, uint32_t bytes) {
    assert(ss->cursor + bytes <= ss->size);
    ss->cursor += bytes;
}

void skip_to(struct serializer *ss, uint32_t new_cursor) {
    assert(new_cursor <= ss->size);
    ss->cursor = new_cursor;
}

uint8_t unpack_uint8(struct serializer *ss) {
    assert(ss->cursor + 1 <= ss->size);
    return ss->base[ss->cursor++];
}

int8_t unpack_int8(struct serializer *ss) {
    return (int8_t) unpack_uint8(ss);
}

uint16_t unpack_uint16(struct serializer *ss) {
    uint16_t partial = unpack_uint8(ss);
    return (partial << 8) | unpack_uint8(ss);
}

int16_t unpack_int16(struct serializer *ss) {
    return (int16_t) unpack_uint16(ss);
}

uint32_t unpack_uint24(struct serializer *ss) {
    uint32_t partial = unpack_uint16(ss);
    return (partial << 8) | unpack_uint8(ss);
}

int32_t unpack_int24(struct serializer *ss) {
    uint32_t bits = unpack_uint24(ss);

    /* sign-extend it from 24 to 32 bits */
    if ((bits & 0x800000) != 0)
        bits |= 0xff000000;
    return (int32_t) bits;
}

uint32_t unpack_uint32(struct serializer *ss) {
    uint32_t partial = unpack_uint16(ss);
    return (partial << 16) | unpack_uint16(ss);
}

int32_t unpack_int32(struct serializer *ss) {
    return (int32_t) unpack_uint32(ss);
}

uint64_t unpack_uint48(struct serializer *ss) {
    uint64_t partial = unpack_uint32(ss);
    return (partial << 16) | unpack_uint16(ss);
}

int64_t unpack_int48(struct serializer *ss) {
    uint64_t bits = unpack_uint48(ss);

    /* sign-extend it from 48 to 64 bits */
    if ((bits & 0x800000000000) != 0)
        bits |= 0xffff000000000000;
    return (int64_t) bits;
}

uint64_t unpack_uint64(struct serializer *ss) {
    uint64_t partial = unpack_uint32(ss);
    return (partial << 32) | unpack_uint32(ss);
}

int64_t unpack_int64(struct serializer *ss) {
    return (int64_t) unpack_uint64(ss);
}

double unpack_double(struct serializer *ss) {
    uint64_t bits = unpack_uint64(ss);
    double result;
    memcpy(&result, &bits, sizeof(bits));

    return result;
}

int64_t unpack_varint(struct serializer *ss) {
    uint64_t partial = 0;
    for (uint32_t i = 0; i < 8; i++) {
        uint8_t nextbyte = unpack_uint8(ss);
        partial = (partial << 7) | (nextbyte & 0x7f);
        if ((nextbyte & 0x80) == 0)
            return partial;
    }

    /* special case: all 8 bits of 9th byte are used */
    partial = (partial << 8) | unpack_uint8(ss);
    return (int64_t) partial;
}

void unpack_raw_string(struct serializer *ss, uint8_t *buffer, uint32_t length) {
    assert(ss->cursor + length <= ss->size);
    memcpy(buffer, ss->base + ss->cursor, length);
    ss->cursor += length;
}

uint8_t *unpack_raw_string_malloc(struct serializer *ss, uint32_t length) {
    uint8_t *result = must_malloc(length + 1);
    unpack_raw_string(ss, result, length);
    result[length] = 0;
    return result;
}

uint8_t *unpack_blob_malloc(struct serializer *ss, uint32_t length) {
    uint8_t *result = must_malloc(length);
    assert(ss->cursor + length <= ss->size);
    memcpy(result, ss->base + ss->cursor, length);
    ss->cursor += length;
    return result;
}
