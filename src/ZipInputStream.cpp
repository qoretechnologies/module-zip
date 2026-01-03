/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file ZipInputStream.cpp ZipInputStream class implementation */
/*
    Qore zip module

    Copyright (C) 2026 Qore Technologies, s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "ZipInputStream.h"

ZipInputStream::ZipInputStream(void* r, const std::string& name, ExceptionSink* xsink)
    : reader(r), entry_name(name), entry_open(false), eof(false), peek_byte(-2) {
    // Open the entry for reading
    int32_t err = mz_zip_reader_entry_open(reader);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-STREAM-ERROR", "failed to open entry '%s' for streaming: error %d",
                              entry_name.c_str(), err);
        return;
    }
    entry_open = true;
}

ZipInputStream::~ZipInputStream() {
    if (entry_open) {
        mz_zip_reader_entry_close(reader);
        entry_open = false;
    }
}

int64 ZipInputStream::read(void* ptr, int64 limit, ExceptionSink* xsink) {
    if (!entry_open) {
        xsink->raiseException("ZIP-STREAM-ERROR", "stream is not open");
        return 0;
    }

    if (eof) {
        return 0;
    }

    char* buf = static_cast<char*>(ptr);
    int64 total_read = 0;

    // First, return any buffered peek byte
    if (peek_byte >= 0) {
        buf[0] = static_cast<char>(peek_byte);
        peek_byte = -2;
        buf++;
        limit--;
        total_read++;
        if (limit <= 0) {
            return total_read;
        }
    }

    // Read from the entry
    int32_t bytes_read = mz_zip_reader_entry_read(reader, buf, static_cast<int32_t>(limit));
    if (bytes_read < 0) {
        xsink->raiseException("ZIP-STREAM-ERROR", "error reading entry '%s': error %d",
                              entry_name.c_str(), bytes_read);
        return 0;
    }

    if (bytes_read == 0) {
        eof = true;
    }

    return total_read + bytes_read;
}

int64 ZipInputStream::peek(ExceptionSink* xsink) {
    if (!entry_open) {
        xsink->raiseException("ZIP-STREAM-ERROR", "stream is not open");
        return -2;
    }

    if (eof) {
        return -1;
    }

    // Return buffered peek byte if available
    if (peek_byte >= 0) {
        return peek_byte;
    }

    // Read one byte and buffer it
    uint8_t byte;
    int32_t bytes_read = mz_zip_reader_entry_read(reader, &byte, 1);
    if (bytes_read < 0) {
        xsink->raiseException("ZIP-STREAM-ERROR", "error peeking entry '%s': error %d",
                              entry_name.c_str(), bytes_read);
        return -2;
    }

    if (bytes_read == 0) {
        eof = true;
        return -1;
    }

    peek_byte = byte;
    return peek_byte;
}
