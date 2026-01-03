/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file ZipOutputStream.cpp ZipOutputStream class implementation */
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

#include "ZipOutputStream.h"

#include <ctime>
#include <cstring>

ZipOutputStream::ZipOutputStream(void* w, const std::string& name,
                                  int16_t compression_method, int16_t compression_level,
                                  ExceptionSink* xsink)
    : writer(w), entry_name(name), entry_open(false), closed(false) {
    // Set compression options
    mz_zip_writer_set_compress_method(writer, compression_method);
    mz_zip_writer_set_compress_level(writer, compression_level);

    // Create file info for the entry
    mz_zip_file file_info;
    memset(&file_info, 0, sizeof(file_info));
    file_info.filename = entry_name.c_str();
    file_info.compression_method = compression_method;
    file_info.modified_date = time(nullptr);

    // Open the entry for writing
    int32_t err = mz_zip_writer_entry_open(writer, &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-STREAM-ERROR", "failed to open entry '%s' for streaming write: error %d",
                              entry_name.c_str(), err);
        return;
    }
    entry_open = true;
}

ZipOutputStream::~ZipOutputStream() {
    if (entry_open && !closed) {
        // Close the entry if not already closed
        mz_zip_writer_entry_close(writer);
        entry_open = false;
    }
}

void ZipOutputStream::close(ExceptionSink* xsink) {
    if (closed) {
        return;
    }

    if (entry_open) {
        int32_t err = mz_zip_writer_entry_close(writer);
        if (err != MZ_OK) {
            xsink->raiseException("ZIP-STREAM-ERROR", "error closing entry '%s': error %d",
                                  entry_name.c_str(), err);
        }
        entry_open = false;
    }

    closed = true;
}

void ZipOutputStream::write(const void* ptr, int64 count, ExceptionSink* xsink) {
    if (closed) {
        xsink->raiseException("ZIP-STREAM-ERROR", "stream is closed");
        return;
    }

    if (!entry_open) {
        xsink->raiseException("ZIP-STREAM-ERROR", "stream is not open");
        return;
    }

    if (count <= 0) {
        return;
    }

    int32_t bytes_written = mz_zip_writer_entry_write(writer, ptr, static_cast<int32_t>(count));
    if (bytes_written < 0) {
        xsink->raiseException("ZIP-STREAM-ERROR", "error writing to entry '%s': error %d",
                              entry_name.c_str(), bytes_written);
    } else if (bytes_written != count) {
        xsink->raiseException("ZIP-STREAM-ERROR", "incomplete write to entry '%s': wrote %d of %lld bytes",
                              entry_name.c_str(), bytes_written, (long long)count);
    }
}
