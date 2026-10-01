/* Copyright (C) 2026 Qore Technologies, s.r.o.
 * SPDX-License-Identifier: MIT
 * Public-API regression tests for the private minizip implementation.
 */
#include "mz.h"
#include "mz_strm.h"
#include "mz_strm_mem.h"
#include "mz_zip.h"
#include <assert.h>
#include <stdio.h>

/* Single-threaded, deterministic allocation-failure injection. */
static int fail_malloc;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size) {
    if (fail_malloc) {
        fail_malloc = 0;
        return NULL;
    }
    return __real_malloc(size);
}

static void comment_test(void) {
    void *zip = mz_zip_create();
    const char *comment = NULL;
    char oversized[UINT16_MAX + 2];
    assert(zip);
    assert(mz_zip_set_comment(NULL, "x") == MZ_PARAM_ERROR);
    assert(mz_zip_set_comment(zip, NULL) == MZ_PARAM_ERROR);
    assert(mz_zip_set_comment(zip, "original") == MZ_OK);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    /* The input may alias the currently owned comment. */
    assert(mz_zip_set_comment(zip, comment) == MZ_OK);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    assert(strcmp(comment, "original") == 0);
    memset(oversized, 'a', sizeof(oversized));
    oversized[sizeof(oversized) - 1] = 0;
    assert(mz_zip_set_comment(zip, oversized) == MZ_PARAM_ERROR);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    assert(strcmp(comment, "original") == 0);
    fail_malloc = 1;
    assert(mz_zip_set_comment(zip, "replacement") == MZ_MEM_ERROR);
    assert(fail_malloc == 0);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    assert(strcmp(comment, "original") == 0);
    oversized[UINT16_MAX] = 0;
    assert(mz_zip_set_comment(zip, oversized) == MZ_OK);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    assert(strlen(comment) == UINT16_MAX);
    assert(mz_zip_set_comment(zip, "") == MZ_OK);
    assert(mz_zip_get_comment(zip, &comment) == MZ_OK);
    assert(*comment == 0);
    assert(mz_zip_close(zip) == MZ_OK);
    mz_zip_delete(&zip);
    assert(!zip);
}

static void reopen_test(void) {
    void *stream = mz_stream_mem_create();
    char data[64];
    memset(data, 'x', sizeof(data));
    assert(stream);
    mz_stream_mem_set_grow_size(stream, 64);
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_CREATE) == MZ_OK);
    assert(mz_stream_mem_write(stream, data, sizeof(data)) == sizeof(data));
    assert(mz_stream_mem_close(stream) == MZ_OK);
    mz_stream_mem_set_grow_size(stream, 8);
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_CREATE) == MZ_OK);
    assert(mz_stream_mem_tell(stream) == 0);
    assert(mz_stream_mem_read(stream, data, 1) == 0);
    assert(mz_stream_mem_write(stream, "new data", 8) == 8);
    assert(mz_stream_mem_seek(stream, 0, MZ_SEEK_SET) == MZ_OK);
    assert(mz_stream_mem_read(stream, data, 8) == 8);
    assert(memcmp(data, "new data", 8) == 0);
    mz_stream_mem_delete(&stream);
}

static void growth_test(void) {
    void *stream = mz_stream_mem_create();
    char data[16];
    assert(stream);
    mz_stream_mem_set_grow_size(stream, 8);
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_CREATE) == MZ_OK);
    assert(mz_stream_mem_write(stream, "original", 8) == 8);
    assert(mz_stream_mem_write(stream, "x", -1) == MZ_PARAM_ERROR);
    assert(mz_stream_mem_write(stream, NULL, 1) == MZ_PARAM_ERROR);
    assert(mz_stream_mem_write(stream, NULL, 0) == 0);
    fail_malloc = 1;
    assert(mz_stream_mem_write(stream, "x", 1) == MZ_BUF_ERROR);
    assert(fail_malloc == 0);
    assert(mz_stream_mem_tell(stream) == 8);
    mz_stream_mem_set_grow_size(stream, INT32_MAX);
    assert(mz_stream_mem_write(stream, "x", 1) == MZ_BUF_ERROR);
    assert(mz_stream_mem_seek(stream, 0, MZ_SEEK_SET) == MZ_OK);
    assert(mz_stream_mem_read(stream, data, sizeof(data)) == 8);
    assert(memcmp(data, "original", 8) == 0);
    mz_stream_mem_set_grow_size(stream, 8);
    assert(mz_stream_mem_write(stream, "extended", 8) == 8);
    assert(mz_stream_mem_seek(stream, 0, MZ_SEEK_SET) == MZ_OK);
    assert(mz_stream_mem_read(stream, data, sizeof(data)) == sizeof(data));
    assert(memcmp(data, "originalextended", sizeof(data)) == 0);
    mz_stream_mem_delete(&stream);
}

static void seek_test(void) {
    void *stream = mz_stream_mem_create();
    assert(stream);
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_CREATE) == MZ_OK);
    assert(mz_stream_mem_write(stream, "abc", 3) == 3);
    for (int origin = MZ_SEEK_SET; origin <= MZ_SEEK_END; ++origin) {
        assert(mz_stream_mem_seek(stream, INT64_MAX, origin) == MZ_SEEK_ERROR);
        assert(mz_stream_mem_seek(stream, INT64_MIN, origin) == MZ_SEEK_ERROR);
        assert(mz_stream_mem_tell(stream) == 3);
    }
    assert(mz_stream_mem_seek(stream, 0, 99) == MZ_SEEK_ERROR);
    assert(mz_stream_mem_seek(stream, -3, MZ_SEEK_END) == MZ_OK);
    assert(mz_stream_mem_tell(stream) == 0);
    assert(mz_stream_mem_seek(stream, 8192, MZ_SEEK_SET) == MZ_OK);
    assert(mz_stream_mem_tell(stream) == 8192);
    mz_stream_mem_delete(&stream);
}

static void fixed_buffer_test(void) {
    char data[4] = "abc";
    char result[4];
    void *stream = mz_stream_mem_create();
    assert(stream);
    mz_stream_mem_set_buffer(stream, data, sizeof(data));
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_READWRITE) == MZ_OK);
    mz_stream_mem_set_buffer(stream, data, -1);
    mz_stream_mem_set_buffer(stream, NULL, 4);
    mz_stream_mem_set_buffer_limit(stream, -1);
    mz_stream_mem_set_buffer_limit(stream, 5);
    assert(mz_stream_mem_read(stream, result, sizeof(result)) == sizeof(result));
    assert(memcmp(data, result, sizeof(data)) == 0);
    assert(mz_stream_mem_write(stream, "x", 1) == 0);
    assert(mz_stream_mem_seek(stream, 5, MZ_SEEK_SET) == MZ_SEEK_ERROR);
    mz_stream_mem_delete(&stream);
    stream = mz_stream_mem_create();
    assert(stream);
    mz_stream_mem_set_grow_size(stream, -1);
    mz_stream_mem_set_grow_size(stream, 0);
    assert(mz_stream_mem_open(stream, NULL, MZ_OPEN_MODE_CREATE) == MZ_OK);
    mz_stream_mem_delete(&stream);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "comment") == 0) {
        comment_test();
    } else if (strcmp(argv[1], "reopen") == 0) {
        reopen_test();
    } else if (strcmp(argv[1], "growth") == 0) {
        growth_test();
    } else if (strcmp(argv[1], "seek") == 0) {
        seek_test();
    } else if (strcmp(argv[1], "fixed-buffer") == 0) {
        fixed_buffer_test();
    } else {
        return 2;
    }
    puts("PASS");
    return 0;
}
