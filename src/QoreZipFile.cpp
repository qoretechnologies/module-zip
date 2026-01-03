/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreZipFile.cpp QoreZipFile class implementation */
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

#include "QoreZipFile.h"
#include "ZipInputStream.h"
#include "ZipOutputStream.h"

#include <cstring>
#include <ctime>
#include <sys/stat.h>

// Forward declarations for class IDs
DLLLOCAL extern qore_classid_t CID_ZIPINPUTSTREAM;
DLLLOCAL extern qore_classid_t CID_ZIPOUTPUTSTREAM;
DLLLOCAL extern QoreClass* QC_ZIPINPUTSTREAM;
DLLLOCAL extern QoreClass* QC_ZIPOUTPUTSTREAM;

// Constructor for file-based archive
QoreZipFile::QoreZipFile(const char* path, ZipMode m, ExceptionSink* xsink)
    : filepath(path), mode(m), reader(nullptr), writer(nullptr), mem_stream(nullptr),
      in_memory(false), closed(false), active_streams(0), max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
    if (mode == ZIP_MODE_READ) {
        openRead(xsink);
    } else {
        openWrite(xsink);
    }
}

// Constructor for in-memory archive (from binary data)
QoreZipFile::QoreZipFile(const BinaryNode* data, ExceptionSink* xsink)
    : mode(ZIP_MODE_READ), reader(nullptr), writer(nullptr), mem_stream(nullptr),
      in_memory(true), closed(false), active_streams(0), max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
    // Create memory stream from binary data
    mem_stream = mz_stream_mem_create();
    if (!mem_stream) {
        xsink->raiseException("ZIP-ERROR", "failed to create memory stream");
        return;
    }

    // Open using buffer API
    reader = mz_zip_reader_create();
    if (!reader) {
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to create zip reader");
        return;
    }

    int32_t err = mz_zip_reader_open_buffer(reader, (const uint8_t*)data->getPtr(), data->size(), 0);
    if (err != MZ_OK) {
        mz_zip_reader_delete(&reader);
        reader = nullptr;
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to open ZIP archive from binary data: error %d", err);
    }
}

// Constructor for new in-memory archive
QoreZipFile::QoreZipFile(ExceptionSink* xsink)
    : mode(ZIP_MODE_WRITE), reader(nullptr), writer(nullptr), mem_stream(nullptr),
      in_memory(true), closed(false), active_streams(0), max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
    // Create memory stream for writing
    mem_stream = mz_stream_mem_create();
    if (!mem_stream) {
        xsink->raiseException("ZIP-ERROR", "failed to create memory stream");
        return;
    }

    mz_stream_mem_set_grow_size(mem_stream, ZIP_MEM_STREAM_GROW_SIZE);
    int32_t err = mz_stream_open(mem_stream, nullptr, MZ_OPEN_MODE_CREATE);
    if (err != MZ_OK) {
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to open memory stream: error %d", err);
        return;
    }

    // Create writer on memory stream
    writer = mz_zip_writer_create();
    if (!writer) {
        mz_stream_close(mem_stream);
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to create zip writer");
        return;
    }

    err = mz_zip_writer_open(writer, mem_stream, 0);
    if (err != MZ_OK) {
        mz_zip_writer_delete(&writer);
        writer = nullptr;
        mz_stream_close(mem_stream);
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to create in-memory ZIP archive: error %d", err);
    }
}

QoreZipFile::~QoreZipFile() {
    ExceptionSink xsink;
    close(&xsink);
}

void QoreZipFile::openRead(ExceptionSink* xsink) {
    reader = mz_zip_reader_create();
    if (!reader) {
        xsink->raiseException("ZIP-ERROR", "failed to create zip reader");
        return;
    }

    int32_t err = mz_zip_reader_open_file(reader, filepath.c_str());
    if (err != MZ_OK) {
        mz_zip_reader_delete(&reader);
        reader = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to open ZIP archive '%s' for reading: error %d",
                              filepath.c_str(), err);
    }
}

void QoreZipFile::openWrite(ExceptionSink* xsink) {
    writer = mz_zip_writer_create();
    if (!writer) {
        xsink->raiseException("ZIP-ERROR", "failed to create zip writer");
        return;
    }

    int32_t err = mz_zip_writer_open_file(writer, filepath.c_str(), 0,
        (mode == ZIP_MODE_APPEND) ? 1 : 0);
    if (err != MZ_OK) {
        mz_zip_writer_delete(&writer);
        writer = nullptr;
        xsink->raiseException("ZIP-ERROR", "failed to open ZIP archive '%s' for writing: error %d",
                              filepath.c_str(), err);
    }
}

void QoreZipFile::close(ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (closed) {
        return;
    }

    // Check for active streams
    if (active_streams > 0) {
        xsink->raiseException("ZIP-ERROR", "cannot close archive with %d active stream(s)", (int)active_streams);
        return;
    }

    if (reader) {
        mz_zip_reader_close(reader);
        mz_zip_reader_delete(&reader);
        reader = nullptr;
    }

    if (writer) {
        mz_zip_writer_close(writer);
        mz_zip_writer_delete(&writer);
        writer = nullptr;
    }

    if (mem_stream) {
        mz_stream_close(mem_stream);
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
    }

    closed = true;
}

BinaryNode* QoreZipFile::toData(ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!in_memory) {
        xsink->raiseException("ZIP-ERROR", "toData() can only be called on in-memory archives");
        return nullptr;
    }

    if (closed) {
        xsink->raiseException("ZIP-ERROR", "archive is already closed");
        return nullptr;
    }

    // Check for active streams
    if (active_streams > 0) {
        xsink->raiseException("ZIP-ERROR", "cannot finalize archive with %d active stream(s)", (int)active_streams);
        return nullptr;
    }

    if (writer) {
        // Close the writer first to finalize the archive
        mz_zip_writer_close(writer);
        mz_zip_writer_delete(&writer);
        writer = nullptr;
    }

    // Get the buffer from the memory stream
    const void* buf = nullptr;
    int32_t buf_size = 0;
    mz_stream_mem_get_buffer(mem_stream, &buf);
    mz_stream_mem_get_buffer_length(mem_stream, &buf_size);

    if (!buf || buf_size <= 0) {
        xsink->raiseException("ZIP-ERROR", "failed to get archive data");
        return nullptr;
    }

    // Check allocation size limit
    if (buf_size > max_alloc_size) {
        xsink->raiseException("ZIP-ERROR", "archive size %d exceeds maximum allocation size %lld",
                              buf_size, (long long)max_alloc_size);
        return nullptr;
    }

    // Make a copy of the data
    void* copy = malloc(buf_size);
    if (!copy) {
        xsink->raiseException("ZIP-ERROR", "failed to allocate memory for archive data");
        return nullptr;
    }
    memcpy(copy, buf, buf_size);

    // Clean up memory stream since we're done with it
    if (mem_stream) {
        mz_stream_close(mem_stream);
        mz_stream_mem_delete(&mem_stream);
        mem_stream = nullptr;
    }

    // Mark as closed since we've finalized the archive
    closed = true;

    return new BinaryNode(copy, buf_size);
}

bool QoreZipFile::checkOpenUnlocked(ExceptionSink* xsink, bool forWrite) {
    if (closed) {
        xsink->raiseException("ZIP-ERROR", "archive is closed");
        return false;
    }

    if (forWrite && !writer) {
        xsink->raiseException("ZIP-ERROR", "archive is not open for writing");
        return false;
    }

    if (!forWrite && !reader) {
        xsink->raiseException("ZIP-ERROR", "archive is not open for reading");
        return false;
    }

    return true;
}

bool QoreZipFile::validateExtractPath(const char* entry_name, const char* dest_path, ExceptionSink* xsink) {
    // Check for path traversal attempts
    if (!entry_name) {
        return true;
    }

    // Check for absolute paths
    if (entry_name[0] == '/') {
        xsink->raiseException("ZIP-SECURITY-ERROR", "absolute path in archive entry: '%s'", entry_name);
        return false;
    }

    // Check for path traversal sequences
    const char* p = entry_name;
    while (*p) {
        // Check for ".." component
        if (p[0] == '.' && p[1] == '.') {
            // Check if it's at the start or after a path separator
            if (p == entry_name || p[-1] == '/') {
                // Check if it's followed by end, slash, or backslash
                if (p[2] == '\0' || p[2] == '/' || p[2] == '\\') {
                    xsink->raiseException("ZIP-SECURITY-ERROR", "path traversal detected in archive entry: '%s'", entry_name);
                    return false;
                }
            }
        }
        // Also check for backslashes (Windows-style paths)
        if (*p == '\\') {
            xsink->raiseException("ZIP-SECURITY-ERROR", "backslash in archive entry path: '%s'", entry_name);
            return false;
        }
        ++p;
    }

    return true;
}

QoreHashNode* QoreZipFile::createEntryInfo(mz_zip_file* file_info, ExceptionSink* xsink) {
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(hashdeclZipEntryInfo, xsink), xsink);

    h->setKeyValue("name", new QoreStringNode(file_info->filename), xsink);
    h->setKeyValue("size", file_info->uncompressed_size, xsink);
    h->setKeyValue("compressed_size", file_info->compressed_size, xsink);

    // Convert time_t to date
    DateTimeNode* dt = DateTimeNode::makeAbsolute(
        currentTZ(),
        (int64)file_info->modified_date,
        0
    );
    h->setKeyValue("modified", dt, xsink);

    h->setKeyValue("crc32", (int64)file_info->crc, xsink);
    h->setKeyValue("compression_method", (int64)file_info->compression_method, xsink);

    // Check if directory (filename ends with /)
    size_t len = strlen(file_info->filename);
    bool is_dir = (len > 0 && file_info->filename[len - 1] == '/');
    h->setKeyValue("is_directory", is_dir, xsink);
    h->setKeyValue("is_encrypted", (bool)(file_info->flag & MZ_ZIP_FLAG_ENCRYPTED), xsink);

    if (file_info->comment && file_info->comment_size > 0) {
        h->setKeyValue("comment", new QoreStringNode(file_info->comment, file_info->comment_size, QCS_UTF8), xsink);
    }

    return h.release();
}

QoreListNode* QoreZipFile::entries(ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    ReferenceHolder<QoreListNode> list(new QoreListNode(hashdeclZipEntryInfo->getTypeInfo(true)), xsink);

    int32_t err = mz_zip_reader_goto_first_entry(reader);
    while (err == MZ_OK) {
        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(reader, &file_info);
        if (err != MZ_OK) {
            break;
        }

        list->push(createEntryInfo(file_info, xsink), xsink);
        err = mz_zip_reader_goto_next_entry(reader);
    }

    if (err != MZ_END_OF_LIST && err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "error reading archive entries: %d", err);
        return nullptr;
    }

    return list.release();
}

int64 QoreZipFile::count(ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return -1;
    }

    int64 count = 0;
    int32_t err = mz_zip_reader_goto_first_entry(reader);
    while (err == MZ_OK) {
        count++;
        err = mz_zip_reader_goto_next_entry(reader);
    }

    return count;
}

bool QoreZipFile::hasEntry(const char* name, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return false;
    }

    int32_t err = mz_zip_reader_locate_entry(reader, name, 0);
    return err == MZ_OK;
}

BinaryNode* QoreZipFile::read(const char* name, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    int32_t err = mz_zip_reader_locate_entry(reader, name, 0);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "entry '%s' not found", name);
        return nullptr;
    }

    mz_zip_file* file_info = nullptr;
    err = mz_zip_reader_entry_get_info(reader, &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to get entry info for '%s'", name);
        return nullptr;
    }

    // Handle empty files
    if (file_info->uncompressed_size == 0) {
        return new BinaryNode();
    }

    // Check allocation size limit
    if ((int64)file_info->uncompressed_size > max_alloc_size) {
        xsink->raiseException("ZIP-ERROR", "entry '%s' size %lld exceeds maximum allocation size %lld",
                              name, (long long)file_info->uncompressed_size, (long long)max_alloc_size);
        return nullptr;
    }

    if (!password.empty()) {
        mz_zip_reader_set_password(reader, password.c_str());
    }

    err = mz_zip_reader_entry_open(reader);
    if (err != MZ_OK) {
        // Provide more specific error for wrong password
        if (file_info->flag & MZ_ZIP_FLAG_ENCRYPTED) {
            xsink->raiseException("ZIP-ERROR", "failed to open encrypted entry '%s' for reading: error %d (wrong password?)", name, err);
        } else {
            xsink->raiseException("ZIP-ERROR", "failed to open entry '%s' for reading: error %d", name, err);
        }
        return nullptr;
    }

    // Allocate buffer
    void* buf = malloc(file_info->uncompressed_size);
    if (!buf) {
        mz_zip_reader_entry_close(reader);
        xsink->raiseException("ZIP-ERROR", "failed to allocate memory for entry '%s'", name);
        return nullptr;
    }

    int32_t bytes_read = mz_zip_reader_entry_read(reader, buf, file_info->uncompressed_size);
    mz_zip_reader_entry_close(reader);

    if (bytes_read < 0) {
        free(buf);
        xsink->raiseException("ZIP-ERROR", "failed to read entry '%s': error %d", name, bytes_read);
        return nullptr;
    }

    return new BinaryNode(buf, bytes_read);
}

QoreStringNode* QoreZipFile::readText(const char* name, const char* encoding, ExceptionSink* xsink) {
    SimpleRefHolder<BinaryNode> bin(read(name, xsink));
    if (*xsink || !bin) {
        return nullptr;
    }

    const QoreEncoding* enc = encoding ? QEM.findCreate(encoding) : QCS_UTF8;
    return new QoreStringNode((const char*)bin->getPtr(), bin->size(), enc);
}

QoreHashNode* QoreZipFile::getEntry(const char* name, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    int32_t err = mz_zip_reader_locate_entry(reader, name, 0);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "entry '%s' not found", name);
        return nullptr;
    }

    mz_zip_file* file_info = nullptr;
    err = mz_zip_reader_entry_get_info(reader, &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to get entry info for '%s'", name);
        return nullptr;
    }

    return createEntryInfo(file_info, xsink);
}

void QoreZipFile::parseAddOptions(const QoreHashNode* opts, int16_t& compression_method, int16_t& compression_level,
                                   std::string& entry_password, std::string& comment, int64& modified_time,
                                   ExceptionSink* xsink) {
    compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    compression_level = MZ_COMPRESS_LEVEL_DEFAULT;
    modified_time = 0;

    if (!opts) {
        return;
    }

    QoreValue v = opts->getKeyValue("compression_method");
    if (!v.isNothing()) {
        compression_method = (int16_t)v.getAsBigInt();
    }

    v = opts->getKeyValue("compression_level");
    if (!v.isNothing()) {
        compression_level = (int16_t)v.getAsBigInt();
    }

    v = opts->getKeyValue("password");
    if (!v.isNothing() && v.getType() == NT_STRING) {
        entry_password = v.get<const QoreStringNode>()->c_str();
    }

    v = opts->getKeyValue("comment");
    if (!v.isNothing() && v.getType() == NT_STRING) {
        comment = v.get<const QoreStringNode>()->c_str();
    }

    v = opts->getKeyValue("modified");
    if (!v.isNothing() && v.getType() == NT_DATE) {
        modified_time = v.get<const DateTimeNode>()->getEpochSecondsUTC();
    }
}

void QoreZipFile::add(const char* name, const BinaryNode* data, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    addUnlocked(name, data, opts, xsink);
}

void QoreZipFile::addUnlocked(const char* name, const BinaryNode* data, const QoreHashNode* opts, ExceptionSink* xsink) {
    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time, xsink);

    mz_zip_file file_info;
    memset(&file_info, 0, sizeof(file_info));
    file_info.filename = name;
    file_info.compression_method = compression_method;
    file_info.modified_date = modified_time ? modified_time : time(nullptr);
    file_info.uncompressed_size = data->size();

    if (!comment.empty()) {
        file_info.comment = comment.c_str();
        file_info.comment_size = (uint16_t)comment.size();
    }

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        mz_zip_writer_set_aes(writer, 1);
    }

    mz_zip_writer_set_compress_method(writer, compression_method);
    mz_zip_writer_set_compress_level(writer, compression_level);

    int32_t err = mz_zip_writer_add_buffer(writer, (void*)data->getPtr(), (int32_t)data->size(), &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to add entry '%s': error %d", name, err);
    }
}

void QoreZipFile::addText(const char* name, const QoreStringNode* text, const char* encoding,
                           const QoreHashNode* opts, ExceptionSink* xsink) {
    // Convert to specified encoding if necessary (can be done without lock)
    TempEncodingHelper teh(text, encoding ? QEM.findCreate(encoding) : QCS_UTF8, xsink);
    if (*xsink) {
        return;
    }

    SimpleRefHolder<BinaryNode> bin(new BinaryNode());
    bin->append(teh->c_str(), teh->size());

    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    addUnlocked(name, *bin, opts, xsink);
}

void QoreZipFile::addFile(const char* name, const char* filepath, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time, xsink);

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        mz_zip_writer_set_aes(writer, 1);
    }

    mz_zip_writer_set_compress_method(writer, compression_method);
    mz_zip_writer_set_compress_level(writer, compression_level);

    // Use add_file instead of add_path
    int32_t err = mz_zip_writer_add_file(writer, filepath, name);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to add file '%s' as '%s': error %d", filepath, name, err);
    }
}

void QoreZipFile::addDirectory(const char* name, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    // Ensure name ends with /
    std::string dir_name = name;
    if (dir_name.empty() || dir_name.back() != '/') {
        dir_name += '/';
    }

    mz_zip_file file_info;
    memset(&file_info, 0, sizeof(file_info));
    file_info.filename = dir_name.c_str();
    file_info.compression_method = MZ_COMPRESS_METHOD_STORE;
    file_info.modified_date = time(nullptr);
    file_info.external_fa = (0x10 << 16);  // Directory attribute for DOS/Windows (high 16 bits)
    file_info.uncompressed_size = 0;
    file_info.compressed_size = 0;

    // Use entry API for directories
    int32_t err = mz_zip_writer_entry_open(writer, &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to add directory '%s': error %d", name, err);
        return;
    }

    err = mz_zip_writer_entry_close(writer);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to close directory entry '%s': error %d", name, err);
    }
}

void QoreZipFile::extractAll(const char* destPath, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return;
    }

    // First, validate all entry paths for security
    int32_t err = mz_zip_reader_goto_first_entry(reader);
    while (err == MZ_OK) {
        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(reader, &file_info);
        if (err == MZ_OK && file_info) {
            if (!validateExtractPath(file_info->filename, destPath, xsink)) {
                return;
            }
        }
        err = mz_zip_reader_goto_next_entry(reader);
    }

    if (opts) {
        QoreValue v = opts->getKeyValue("password");
        if (!v.isNothing() && v.getType() == NT_STRING) {
            mz_zip_reader_set_password(reader, v.get<const QoreStringNode>()->c_str());
        }
    }

    err = mz_zip_reader_save_all(reader, destPath);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to extract archive to '%s': error %d", destPath, err);
    }
}

void QoreZipFile::extractEntry(const char* name, const char* destPath, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return;
    }

    // Validate path for security
    if (!validateExtractPath(name, destPath, xsink)) {
        return;
    }

    int32_t err = mz_zip_reader_locate_entry(reader, name, 0);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "entry '%s' not found", name);
        return;
    }

    if (!password.empty()) {
        mz_zip_reader_set_password(reader, password.c_str());
    }

    err = mz_zip_reader_entry_save_file(reader, destPath);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to extract entry '%s' to '%s': error %d", name, destPath, err);
    }
}

void QoreZipFile::deleteEntry(const char* name, ExceptionSink* xsink) {
    // Note: minizip-ng doesn't support in-place deletion
    // This would require rewriting the archive without the entry
    xsink->raiseException("ZIP-NOT-SUPPORTED", "delete operation is not supported by this implementation; "
                          "to remove entries, create a new archive without the unwanted entries");
}

QoreStringNode* QoreZipFile::getPath() const {
    if (filepath.empty()) {
        return nullptr;
    }
    return new QoreStringNode(filepath);
}

QoreStringNode* QoreZipFile::getComment(ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    const char* comment = nullptr;
    int32_t err = mz_zip_reader_get_comment(reader, &comment);
    if (err != MZ_OK || !comment) {
        return nullptr;
    }

    return new QoreStringNode(comment);
}

void QoreZipFile::setComment(const char* comment, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    mz_zip_writer_set_comment(writer, comment);
}

// QoreZipEntry implementation

QoreZipEntry::QoreZipEntry(const std::string& n, int64 s, int64 cs, int64 m, int64 c,
                           int cm, bool dir, bool enc, const std::string& com)
    : name(n), size(s), compressed_size(cs), modified(m), crc32(c),
      compression_method(cm), is_dir(dir), is_encrypted(enc), comment(com) {
}

QoreZipEntry::~QoreZipEntry() {
}

QoreStringNode* QoreZipEntry::getName() const {
    return new QoreStringNode(name);
}

DateTimeNode* QoreZipEntry::getModified() const {
    return DateTimeNode::makeAbsolute(currentTZ(), modified, 0);
}

QoreStringNode* QoreZipEntry::getComment() const {
    if (comment.empty()) {
        return nullptr;
    }
    return new QoreStringNode(comment);
}

QoreObject* QoreZipFile::openInputStream(const char* name, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    // Locate the entry
    int32_t err = mz_zip_reader_locate_entry(reader, name, 0);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "entry '%s' not found", name);
        return nullptr;
    }

    if (!password.empty()) {
        mz_zip_reader_set_password(reader, password.c_str());
    }

    // Increment active stream count
    ++active_streams;

    // Create the stream - it will open the entry
    ReferenceHolder<ZipInputStream> stream(new ZipInputStream(this, reader, name, xsink), xsink);
    if (*xsink) {
        --active_streams;
        return nullptr;
    }

    return new QoreObject(QC_ZIPINPUTSTREAM, getProgram(), stream.release());
}

QoreObject* QoreZipFile::openOutputStream(const char* name, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return nullptr;
    }

    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time, xsink);

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        mz_zip_writer_set_aes(writer, 1);
    }

    // Increment active stream count
    ++active_streams;

    // Create the stream - it will open the entry
    ReferenceHolder<ZipOutputStream> stream(
        new ZipOutputStream(this, writer, name, compression_method, compression_level, xsink), xsink);
    if (*xsink) {
        --active_streams;
        return nullptr;
    }

    return new QoreObject(QC_ZIPOUTPUTSTREAM, getProgram(), stream.release());
}
