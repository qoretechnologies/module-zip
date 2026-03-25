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
QoreZipFile::QoreZipFile(const char* path, ZipMode m, ExceptionSink* xsink, bool recover)
    : filepath(path), mode(m), reader(nullptr), writer(nullptr), mem_stream(nullptr),
      in_memory(false), closed(false), recover(recover), active_streams(0),
      max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
    if (mode == ZIP_MODE_READ) {
        openRead(xsink);
    } else {
        openWrite(xsink);
    }
}

// Constructor for in-memory archive (from binary data)
QoreZipFile::QoreZipFile(const BinaryNode* data, ExceptionSink* xsink, bool recover)
    : mode(ZIP_MODE_READ), reader(nullptr), writer(nullptr), mem_stream(nullptr),
      in_memory(true), closed(false), recover(recover), active_streams(0),
      max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
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

    if (recover) {
        mz_zip_reader_set_recover(reader, 1);
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
      in_memory(true), closed(false), recover(false), active_streams(0),
      max_alloc_size(ZIP_DEFAULT_MAX_ALLOC_SIZE) {
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
    // Check filesystem sandbox access
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(filepath.c_str(), QSEC_READ, xsink)) {
        return;
    }

    // Check for interrupt before file I/O
    if (qore_check_cancel(xsink, "opening ZIP archive for reading")) {
        return;
    }

    reader = mz_zip_reader_create();
    if (!reader) {
        xsink->raiseException("ZIP-ERROR", "failed to create zip reader");
        return;
    }

    if (recover) {
        mz_zip_reader_set_recover(reader, 1);
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
    // Check filesystem sandbox access (need write and create for new files)
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(filepath.c_str(), QSEC_WRITE | QSEC_CREATE, xsink)) {
        return;
    }

    // Check for interrupt before file I/O
    if (qore_check_cancel(xsink, "opening ZIP archive for writing")) {
        return;
    }

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

std::string QoreZipFile::normalizePath(const std::string& path) {
    std::vector<std::string> components;
    std::istringstream ss(path);
    std::string component;
    while (std::getline(ss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;
        }
        if (component == "..") {
            if (!components.empty()) {
                components.pop_back();
            }
        } else {
            components.push_back(component);
        }
    }
    std::string result;
    if (!path.empty() && path[0] == '/') {
        result = "/";
    }
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) {
            result += "/";
        }
        result += components[i];
    }
    return result;
}

bool QoreZipFile::validateSymlink(const char* entry_name, const char* link_target,
                                   const char* dest_path, ExceptionSink* xsink) {
    if (!link_target || !*link_target) {
        return true;
    }

    // Absolute symlink targets always escape the destination
    if (link_target[0] == '/') {
        xsink->raiseException("ZIP-SECURITY-ERROR",
            "symlink entry '%s' has absolute target '%s'", entry_name, link_target);
        return false;
    }

    // Resolve the target relative to the entry's parent directory within dest_path
    // entry_name = "subdir/link", dest_path = "/dest"
    // parent = "/dest/subdir", resolved = "/dest/subdir/" + link_target
    std::string entry_path = std::string(dest_path) + "/" + entry_name;
    // Get parent directory of the entry
    size_t last_slash = entry_path.rfind('/');
    std::string parent_dir = (last_slash != std::string::npos)
        ? entry_path.substr(0, last_slash) : std::string(dest_path);

    std::string resolved = parent_dir + "/" + link_target;
    std::string normalized = normalizePath(resolved);
    std::string norm_dest = normalizePath(dest_path);

    // Ensure normalized path starts with the destination
    if (normalized.compare(0, norm_dest.size(), norm_dest) != 0
            || (normalized.size() > norm_dest.size() && normalized[norm_dest.size()] != '/')) {
        xsink->raiseException("ZIP-SECURITY-ERROR",
            "symlink entry '%s' target '%s' resolves outside destination directory '%s'",
            entry_name, link_target, dest_path);
        return false;
    }

    return true;
}

bool QoreZipFile::isSymlinkEntry() {
    void* zip_handle = nullptr;
    if (mz_zip_reader_get_zip_handle(reader, &zip_handle) != MZ_OK || !zip_handle) {
        return false;
    }
    return mz_zip_entry_is_symlink(zip_handle) == MZ_OK;
}

std::string QoreZipFile::getSymlinkTarget(ExceptionSink* xsink) {
    // First check the linkname from the extra field
    mz_zip_file* file_info = nullptr;
    int32_t err = mz_zip_reader_entry_get_info(reader, &file_info);
    if (err != MZ_OK || !file_info) {
        return std::string();
    }

    if (file_info->linkname && file_info->linkname[0] != '\0') {
        return std::string(file_info->linkname);
    }

    // Otherwise, the symlink target is stored as the entry's data content
    if (file_info->uncompressed_size == 0 || file_info->uncompressed_size >= UINT16_MAX) {
        return std::string();
    }

    if (!password.empty()) {
        mz_zip_reader_set_password(reader, password.c_str());
    }

    err = mz_zip_reader_entry_open(reader);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to open symlink entry for reading: error %d", err);
        return std::string();
    }

    std::vector<char> buf(file_info->uncompressed_size + 1, 0);
    int32_t bytes_read = mz_zip_reader_entry_read(reader, buf.data(), (int32_t)file_info->uncompressed_size);
    mz_zip_reader_entry_close(reader);

    if (bytes_read < 0) {
        xsink->raiseException("ZIP-ERROR", "failed to read symlink target: error %d", bytes_read);
        return std::string();
    }

    buf[bytes_read] = '\0';
    return std::string(buf.data());
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

    // Encryption method
    int64 enc_method = 0;
    if (file_info->flag & MZ_ZIP_FLAG_ENCRYPTED) {
        if (file_info->aes_version) {
            // AES encryption — map strength to our constants
            switch (file_info->aes_strength) {
                case MZ_AES_STRENGTH_128: enc_method = ZIP_EM_AES_128; break;
                case MZ_AES_STRENGTH_192: enc_method = ZIP_EM_AES_192; break;
                case MZ_AES_STRENGTH_256: enc_method = ZIP_EM_AES_256; break;
                default: enc_method = ZIP_EM_AES_256; break;
            }
        } else {
            enc_method = ZIP_EM_TRAD_PKWARE;
        }
    }
    h->setKeyValue("encryption_method", enc_method, xsink);

    // Symlink detection via Unix permission bits in external_fa
    uint8_t src_sys = MZ_HOST_SYSTEM(file_info->version_madeby);
    bool is_symlink = false;
    if (src_sys == MZ_HOST_SYSTEM_UNIX || src_sys == MZ_HOST_SYSTEM_OSX_DARWIN) {
        uint32_t unix_mode = (file_info->external_fa >> 16) & 0xFFFF;
        is_symlink = (unix_mode & 0170000) == 0120000;  // S_IFLNK
    }
    h->setKeyValue("is_symlink", is_symlink, xsink);

    // Unix permissions
    int64 perms = -1;
    if (src_sys == MZ_HOST_SYSTEM_UNIX || src_sys == MZ_HOST_SYSTEM_OSX_DARWIN) {
        perms = (file_info->external_fa >> 16) & 07777;
    }
    h->setKeyValue("permissions", perms, xsink);

    // Created-by system
    h->setKeyValue("created_by_system", (int64)src_sys, xsink);

    // Symlink target
    if (is_symlink && file_info->linkname && file_info->linkname[0] != '\0') {
        h->setKeyValue("link_target", new QoreStringNode(file_info->linkname), xsink);
    }

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

    // Check for interrupt before iterating entries
    if (qore_check_cancel(xsink, "listing ZIP archive entries")) {
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

    // Check for interrupt before iterating entries
    if (qore_check_cancel(xsink, "counting ZIP archive entries")) {
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

    // Check for interrupt before reading entry data
    if (qore_check_cancel(xsink, "reading ZIP archive entry")) {
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
                                   int& encryption_method, ExceptionSink* xsink) {
    compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    compression_level = MZ_COMPRESS_LEVEL_DEFAULT;
    modified_time = 0;
    encryption_method = -1;  // -1 = not set (use default AES-256 when password provided)

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

    v = opts->getKeyValue("encryption_method");
    if (!v.isNothing()) {
        encryption_method = (int)v.getAsBigInt();
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
    // Check for interrupt before adding entry
    if (qore_check_cancel(xsink, "adding entry to ZIP archive")) {
        return;
    }

    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    int encryption_method;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time,
                     encryption_method, xsink);

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

        // Determine encryption method
        if (encryption_method == ZIP_EM_NONE) {
            // Explicitly no encryption
            mz_zip_writer_set_aes(writer, 0);
        } else if (encryption_method == ZIP_EM_TRAD_PKWARE) {
            mz_zip_writer_set_aes(writer, 0);
        } else {
            // AES encryption (default or explicit)
            mz_zip_writer_set_aes(writer, 1);
            file_info.aes_version = MZ_AES_VERSION;
            // Set AES strength for entry
            if (encryption_method == ZIP_EM_AES_128) {
                file_info.aes_strength = MZ_AES_STRENGTH_128;
            } else if (encryption_method == ZIP_EM_AES_192) {
                file_info.aes_strength = MZ_AES_STRENGTH_192;
            }
            // AES-256 is the default (set by minizip-ng when aes_strength is 0)
        }
    } else {
        // No password for this entry — clear writer encryption state from any previous entry
        mz_zip_writer_set_password(writer, nullptr);
        mz_zip_writer_set_aes(writer, 0);
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

    // Check filesystem sandbox access before reading source file
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(filepath, QSEC_READ, xsink)) {
        return;
    }

    // Check for interrupt before file I/O
    if (qore_check_cancel(xsink, "adding file to ZIP archive")) {
        return;
    }

    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    int encryption_method;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time,
                     encryption_method, xsink);

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        if (encryption_method == ZIP_EM_TRAD_PKWARE) {
            mz_zip_writer_set_aes(writer, 0);
        } else if (encryption_method != ZIP_EM_NONE) {
            mz_zip_writer_set_aes(writer, 1);
        }
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

// Callback data for overwrite and entry callbacks
struct QoreExtractCallbackData {
    bool overwrite;
    ResolvedCallReferenceNode* entry_callback;
    ExceptionSink* xsink;
};

// Static overwrite callback for minizip-ng
static int32_t zip_overwrite_cb(void* handle, void* userdata, mz_zip_file* file_info, const char* path) {
    auto* data = static_cast<QoreExtractCallbackData*>(userdata);
    return data->overwrite ? MZ_OK : MZ_EXIST_ERROR;
}

static int32_t zip_entry_cb(void* handle, void* userdata, mz_zip_file* file_info, const char* path) {
    auto* data = static_cast<QoreExtractCallbackData*>(userdata);
    if (data->entry_callback && file_info && !*data->xsink) {
        ReferenceHolder<QoreListNode> args(new QoreListNode(stringTypeInfo), data->xsink);
        args->push(new QoreStringNode(file_info->filename), data->xsink);
        data->entry_callback->execValue(*args, data->xsink).discard(data->xsink);
    }
    return MZ_OK;
}

void QoreZipFile::parseExtractOptions(const QoreHashNode* opts, std::string& pwd, bool& overwrite,
                                       bool& preserve_paths, bool& allow_symlinks,
                                       std::string& strip_prefix, std::string& add_prefix,
                                       ResolvedCallReferenceNode*& entry_callback,
                                       ExceptionSink* xsink) {
    pwd.clear();
    overwrite = false;
    preserve_paths = true;
    allow_symlinks = false;
    strip_prefix.clear();
    add_prefix.clear();
    entry_callback = nullptr;

    if (!opts) {
        return;
    }

    QoreValue v = opts->getKeyValue("password");
    if (!v.isNothing() && v.getType() == NT_STRING) {
        pwd = v.get<const QoreStringNode>()->c_str();
    }

    v = opts->getKeyValue("overwrite");
    if (!v.isNothing()) {
        overwrite = v.getAsBool();
    }

    v = opts->getKeyValue("preserve_paths");
    if (!v.isNothing()) {
        preserve_paths = v.getAsBool();
    }

    v = opts->getKeyValue("allow_symlinks");
    if (!v.isNothing()) {
        allow_symlinks = v.getAsBool();
    }

    v = opts->getKeyValue("strip_prefix");
    if (!v.isNothing() && v.getType() == NT_STRING) {
        strip_prefix = v.get<const QoreStringNode>()->c_str();
    }

    v = opts->getKeyValue("add_prefix");
    if (!v.isNothing() && v.getType() == NT_STRING) {
        add_prefix = v.get<const QoreStringNode>()->c_str();
    }

    v = opts->getKeyValue("entry_callback");
    if (!v.isNothing() && (v.getType() == NT_FUNCREF || v.getType() == NT_RUNTIME_CLOSURE)) {
        entry_callback = const_cast<ResolvedCallReferenceNode*>(v.get<const ResolvedCallReferenceNode>());
    }
}

void QoreZipFile::setupCallbacks(QoreExtractCallbackData& cb_data) {
    mz_zip_reader_set_overwrite_cb(reader, &cb_data, zip_overwrite_cb);
    if (cb_data.entry_callback) {
        mz_zip_reader_set_entry_cb(reader, &cb_data, zip_entry_cb);
    }
}

void QoreZipFile::clearCallbacks() {
    mz_zip_reader_set_overwrite_cb(reader, nullptr, nullptr);
    mz_zip_reader_set_entry_cb(reader, nullptr, nullptr);
}

int32_t QoreZipFile::extractCurrentEntry(const char* destPath, const char* entry_name, bool preserve_paths,
                                          const std::string& strip_prefix, const std::string& add_prefix,
                                          std::string& out_path, ExceptionSink* xsink) {
    std::string resolved_name = entry_name;

    // Apply strip_prefix
    if (!strip_prefix.empty()) {
        if (resolved_name.compare(0, strip_prefix.size(), strip_prefix) == 0) {
            resolved_name = resolved_name.substr(strip_prefix.size());
            // Remove leading slash after stripping
            if (!resolved_name.empty() && resolved_name[0] == '/') {
                resolved_name = resolved_name.substr(1);
            }
        }
    }

    // Apply preserve_paths: strip directory components if false
    if (!preserve_paths) {
        size_t pos = resolved_name.rfind('/');
        if (pos != std::string::npos) {
            resolved_name = resolved_name.substr(pos + 1);
        }
    }

    // Skip empty names (can happen after stripping)
    if (resolved_name.empty()) {
        return MZ_OK;
    }

    // Apply add_prefix
    if (!add_prefix.empty()) {
        std::string prefix = add_prefix;
        // Ensure prefix ends with / for proper path joining
        if (prefix.back() != '/') {
            prefix += '/';
        }
        resolved_name = prefix + resolved_name;
    }

    // Validate the remapped path for security
    if (!validateExtractPath(resolved_name.c_str(), destPath, xsink)) {
        return MZ_PARAM_ERROR;
    }

    // Construct full output path
    out_path = destPath;
    if (!out_path.empty() && out_path.back() != '/') {
        out_path += '/';
    }
    out_path += resolved_name;

    return mz_zip_reader_entry_save_file(reader, out_path.c_str());
}

void QoreZipFile::addSymlink(const char* name, const char* target, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    mz_zip_file file_info;
    memset(&file_info, 0, sizeof(file_info));
    file_info.filename = name;
    file_info.linkname = target;
    file_info.compression_method = MZ_COMPRESS_METHOD_STORE;
    file_info.modified_date = time(nullptr);
    // Set Unix symlink attribute (0120000 = S_IFLNK) in upper 16 bits of external_fa
    file_info.external_fa = (0120777 << 16);
    file_info.version_madeby = (3 << 8);  // Unix

    int32_t err = mz_zip_writer_entry_open(writer, &file_info);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to add symlink entry '%s': error %d", name, err);
        return;
    }

    // Write the symlink target as the entry's data content
    err = mz_zip_writer_entry_write(writer, target, strlen(target));
    if (err < 0) {
        xsink->raiseException("ZIP-ERROR", "failed to write symlink target for '%s': error %d", name, err);
    }

    mz_zip_writer_entry_close(writer);
}

QoreListNode* QoreZipFile::verify(const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    if (qore_check_cancel(xsink, "verifying ZIP archive")) {
        return nullptr;
    }

    // Apply password if provided
    if (opts) {
        QoreValue v = opts->getKeyValue("password");
        if (!v.isNothing() && v.getType() == NT_STRING) {
            mz_zip_reader_set_password(reader, v.get<const QoreStringNode>()->c_str());
        }
    }

    ReferenceHolder<QoreListNode> results(new QoreListNode(hashdeclZipVerifyResult->getTypeInfo(true)), xsink);

    char buf[65536];
    int32_t err = mz_zip_reader_goto_first_entry(reader);
    while (err == MZ_OK) {
        if (qore_check_cancel(xsink, "verifying ZIP archive entry")) {
            return nullptr;
        }

        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(reader, &file_info);
        if (err != MZ_OK) {
            break;
        }

        // Skip directories
        size_t len = strlen(file_info->filename);
        if (len > 0 && file_info->filename[len - 1] == '/') {
            err = mz_zip_reader_goto_next_entry(reader);
            continue;
        }

        ReferenceHolder<QoreHashNode> result(new QoreHashNode(hashdeclZipVerifyResult, xsink), xsink);
        result->setKeyValue("name", new QoreStringNode(file_info->filename), xsink);

        // Open and read the entry to trigger CRC verification
        int32_t open_err = mz_zip_reader_entry_open(reader);
        if (open_err != MZ_OK) {
            result->setKeyValue("valid", false, xsink);
            QoreStringNode* err_str = new QoreStringNode();
            err_str->sprintf("failed to open entry: error %d", open_err);
            result->setKeyValue("error", err_str, xsink);
            results->push(result.release(), xsink);
            err = mz_zip_reader_goto_next_entry(reader);
            continue;
        }

        // Read all data (discarding it) to trigger CRC check
        bool entry_valid = true;
        QoreStringNode* error_msg = nullptr;
        while (true) {
            int32_t bytes_read = mz_zip_reader_entry_read(reader, buf, sizeof(buf));
            if (bytes_read == 0) {
                break;  // EOF
            }
            if (bytes_read < 0) {
                entry_valid = false;
                error_msg = new QoreStringNode();
                error_msg->sprintf("read error: %d", bytes_read);
                break;
            }
        }

        int32_t close_err = mz_zip_reader_entry_close(reader);
        if (close_err != MZ_OK && entry_valid) {
            entry_valid = false;
            if (close_err == MZ_CRC_ERROR) {
                error_msg = new QoreStringNode("CRC mismatch");
            } else {
                error_msg = new QoreStringNode();
                error_msg->sprintf("close error: %d", close_err);
            }
        }

        result->setKeyValue("valid", entry_valid, xsink);
        if (!entry_valid && error_msg) {
            result->setKeyValue("error", error_msg, xsink);
        }

        results->push(result.release(), xsink);
        err = mz_zip_reader_goto_next_entry(reader);
    }

    if (err != MZ_END_OF_LIST && err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "error iterating archive entries: %d", err);
        return nullptr;
    }

    return results.release();
}

void QoreZipFile::extractAll(const char* destPath, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return;
    }

    // Check filesystem sandbox access before writing to destination
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(destPath, QSEC_WRITE | QSEC_CREATE, xsink)) {
        return;
    }

    // Check for interrupt before extraction
    if (qore_check_cancel(xsink, "extracting ZIP archive")) {
        return;
    }

    // Parse options
    std::string pwd, strip_prefix, add_prefix;
    bool overwrite, preserve_paths, allow_symlinks;
    ResolvedCallReferenceNode* entry_callback = nullptr;
    parseExtractOptions(opts, pwd, overwrite, preserve_paths, allow_symlinks, strip_prefix, add_prefix,
                         entry_callback, xsink);

    if (!pwd.empty()) {
        mz_zip_reader_set_password(reader, pwd.c_str());
    }

    // Set up callbacks (stack-local data — thread safe)
    QoreExtractCallbackData cb_data{overwrite, entry_callback, xsink};
    setupCallbacks(cb_data);

    bool need_manual_loop = !preserve_paths || !strip_prefix.empty() || !add_prefix.empty();

    int32_t err;
    if (need_manual_loop) {
        // Manual iteration for path manipulation
        err = mz_zip_reader_goto_first_entry(reader);
        while (err == MZ_OK) {
            if (qore_check_cancel(xsink, "extracting ZIP archive")) {
                break;
            }

            mz_zip_file* file_info = nullptr;
            err = mz_zip_reader_entry_get_info(reader, &file_info);
            if (err != MZ_OK) {
                break;
            }

            // Validate original path for security
            if (!validateExtractPath(file_info->filename, destPath, xsink)) {
                break;
            }

            // Symlink handling
            if (isSymlinkEntry()) {
                if (!allow_symlinks) {
                    // Skip symlinks by default
                    err = mz_zip_reader_goto_next_entry(reader);
                    continue;
                }
                // Validate symlink target
                std::string target = getSymlinkTarget(xsink);
                if (*xsink) {
                    break;
                }
                if (!target.empty() && !validateSymlink(file_info->filename, target.c_str(), destPath, xsink)) {
                    break;
                }
            }

            std::string out_path;
            int32_t save_err = extractCurrentEntry(destPath, file_info->filename, preserve_paths, strip_prefix,
                                                    add_prefix, out_path, xsink);
            if (*xsink) {
                break;
            }

            if (save_err != MZ_OK && save_err != MZ_EXIST_ERROR) {
                xsink->raiseException("ZIP-ERROR", "failed to extract entry '%s' to '%s': error %d",
                                      file_info->filename, out_path.c_str(), save_err);
                break;
            }

            err = mz_zip_reader_goto_next_entry(reader);
        }

        if (err == MZ_END_OF_LIST) {
            err = MZ_OK;
        }
    } else {
        // First, validate all entry paths and symlinks for security
        err = mz_zip_reader_goto_first_entry(reader);
        while (err == MZ_OK) {
            mz_zip_file* file_info = nullptr;
            err = mz_zip_reader_entry_get_info(reader, &file_info);
            if (err == MZ_OK && file_info) {
                if (!validateExtractPath(file_info->filename, destPath, xsink)) {
                    clearCallbacks();
                    return;
                }
                // Check symlinks in the save_all path too
                if (isSymlinkEntry()) {
                    if (!allow_symlinks) {
                        // For save_all path, we can't skip individual entries — fall back to manual loop
                        // Reset and use manual loop instead
                        clearCallbacks();
                        // Re-parse is safe; re-enter with manual loop forced
                        setupCallbacks(cb_data);
                        need_manual_loop = true;
                        break;
                    }
                    std::string target = getSymlinkTarget(xsink);
                    if (*xsink) {
                        clearCallbacks();
                        return;
                    }
                    if (!target.empty() && !validateSymlink(file_info->filename, target.c_str(), destPath, xsink)) {
                        clearCallbacks();
                        return;
                    }
                }
            }
            err = mz_zip_reader_goto_next_entry(reader);
        }

        if (need_manual_loop) {
            // Symlinks detected — re-extract with manual loop to skip them
            err = mz_zip_reader_goto_first_entry(reader);
            while (err == MZ_OK) {
                if (qore_check_cancel(xsink, "extracting ZIP archive")) {
                    break;
                }

                mz_zip_file* file_info = nullptr;
                err = mz_zip_reader_entry_get_info(reader, &file_info);
                if (err != MZ_OK) {
                    break;
                }

                // Skip symlinks
                if (isSymlinkEntry()) {
                    err = mz_zip_reader_goto_next_entry(reader);
                    continue;
                }

                std::string out_path = std::string(destPath) + "/" + file_info->filename;
                int32_t save_err = mz_zip_reader_entry_save_file(reader, out_path.c_str());
                if (save_err != MZ_OK && save_err != MZ_EXIST_ERROR) {
                    xsink->raiseException("ZIP-ERROR", "failed to extract entry '%s': error %d",
                                          file_info->filename, save_err);
                    break;
                }

                err = mz_zip_reader_goto_next_entry(reader);
            }

            if (err == MZ_END_OF_LIST) {
                err = MZ_OK;
            }
        } else {
            err = mz_zip_reader_save_all(reader, destPath);
        }
    }

    // Clean up callbacks
    clearCallbacks();

    if (!*xsink && err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to extract archive to '%s': error %d", destPath, err);
    }
}

QoreListNode* QoreZipFile::extractEntries(const char* destPath, const QoreListNode* entryNames,
                                           const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return nullptr;
    }

    // Check filesystem sandbox access before writing to destination
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(destPath, QSEC_WRITE | QSEC_CREATE, xsink)) {
        return nullptr;
    }

    // Check for interrupt before extraction
    if (qore_check_cancel(xsink, "extracting ZIP archive entries")) {
        return nullptr;
    }

    // Parse options
    std::string pwd, strip_prefix, add_prefix;
    bool overwrite, preserve_paths, allow_symlinks;
    ResolvedCallReferenceNode* entry_callback = nullptr;
    parseExtractOptions(opts, pwd, overwrite, preserve_paths, allow_symlinks, strip_prefix, add_prefix,
                         entry_callback, xsink);

    if (!pwd.empty()) {
        mz_zip_reader_set_password(reader, pwd.c_str());
    }

    // Build set of requested entry names for O(1) lookup
    std::unordered_set<std::string> name_set;
    if (entryNames) {
        ConstListIterator li(entryNames);
        while (li.next()) {
            QoreValue v = li.getValue();
            if (v.getType() == NT_STRING) {
                name_set.insert(v.get<const QoreStringNode>()->c_str());
            }
        }
        // If the list was provided but empty, return empty result immediately
        if (name_set.empty()) {
            return new QoreListNode(stringTypeInfo);
        }
    }

    // Set up callbacks (stack-local data — thread safe)
    QoreExtractCallbackData cb_data{overwrite, entry_callback, xsink};
    setupCallbacks(cb_data);

    ReferenceHolder<QoreListNode> extracted(new QoreListNode(stringTypeInfo), xsink);

    // Iterate all entries and extract matching ones
    int32_t err = mz_zip_reader_goto_first_entry(reader);
    while (err == MZ_OK) {
        if (qore_check_cancel(xsink, "extracting ZIP archive entries")) {
            break;
        }

        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(reader, &file_info);
        if (err != MZ_OK) {
            break;
        }

        // Check if this entry is in the requested set
        if (!name_set.empty() && name_set.find(file_info->filename) == name_set.end()) {
            err = mz_zip_reader_goto_next_entry(reader);
            continue;
        }

        // Validate original path for security
        if (!validateExtractPath(file_info->filename, destPath, xsink)) {
            break;
        }

        // Symlink handling
        if (isSymlinkEntry()) {
            if (!allow_symlinks) {
                err = mz_zip_reader_goto_next_entry(reader);
                continue;
            }
            std::string target = getSymlinkTarget(xsink);
            if (*xsink) {
                break;
            }
            if (!target.empty() && !validateSymlink(file_info->filename, target.c_str(), destPath, xsink)) {
                break;
            }
        }

        std::string out_path;
        int32_t save_err = extractCurrentEntry(destPath, file_info->filename, preserve_paths, strip_prefix,
                                                add_prefix, out_path, xsink);
        if (*xsink) {
            break;
        }

        if (save_err == MZ_OK) {
            // Check if it's not a directory
            size_t len = strlen(file_info->filename);
            bool is_dir = (len > 0 && file_info->filename[len - 1] == '/');
            if (!is_dir) {
                extracted->push(new QoreStringNode(out_path), xsink);
            }
        } else if (save_err != MZ_EXIST_ERROR) {
            // MZ_EXIST_ERROR means overwrite was denied; that's OK, skip it
            xsink->raiseException("ZIP-ERROR", "failed to extract entry '%s' to '%s': error %d",
                                  file_info->filename, out_path.c_str(), save_err);
            break;
        }

        err = mz_zip_reader_goto_next_entry(reader);
    }

    // Clean up callbacks
    clearCallbacks();

    if (*xsink) {
        return nullptr;
    }

    if (err != MZ_END_OF_LIST && err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "error iterating archive entries: %d", err);
        return nullptr;
    }

    return extracted.release();
}

void QoreZipFile::addPath(const char* path, const char* root_path, const QoreHashNode* opts, ExceptionSink* xsink) {
    QoreAutoRWWriteLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, true)) {
        return;
    }

    // Check filesystem sandbox access before reading source
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(path, QSEC_READ, xsink)) {
        return;
    }

    // Check for interrupt before I/O
    if (qore_check_cancel(xsink, "adding path to ZIP archive")) {
        return;
    }

    int16_t compression_method, compression_level;
    std::string entry_password, comment;
    int64 modified_time;
    int encryption_method;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time,
                     encryption_method, xsink);

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        if (encryption_method == ZIP_EM_TRAD_PKWARE) {
            mz_zip_writer_set_aes(writer, 0);
        } else if (encryption_method != ZIP_EM_NONE) {
            mz_zip_writer_set_aes(writer, 1);
        }
    }

    mz_zip_writer_set_compress_method(writer, compression_method);
    mz_zip_writer_set_compress_level(writer, compression_level);

    int32_t err = mz_zip_writer_add_path(writer, path, root_path, 1 /*include_path*/, 1 /*recursive*/);
    if (err != MZ_OK) {
        xsink->raiseException("ZIP-ERROR", "failed to add path '%s': error %d", path, err);
    }
}

void QoreZipFile::extractEntry(const char* name, const char* destPath, ExceptionSink* xsink) {
    QoreAutoRWReadLocker lock(rwlock);

    if (!checkOpenUnlocked(xsink, false)) {
        return;
    }

    // Check filesystem sandbox access before writing to destination
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(destPath, QSEC_WRITE | QSEC_CREATE, xsink)) {
        return;
    }

    // Validate path for security
    if (!validateExtractPath(name, destPath, xsink)) {
        return;
    }

    // Check for interrupt before extraction
    if (qore_check_cancel(xsink, "extracting ZIP archive entry")) {
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
    // Use replaceEntries() for deletion
    xsink->raiseException("ZIP-NOT-SUPPORTED", "use ZipFile::replaceEntries() for entry deletion; "
                          "in-place deletion is not supported by the underlying library");
}

QoreHashNode* QoreZipFile::replaceEntries(const char* archive_path, const QoreHashNode* replacements,
                                           const QoreListNode* delete_names, ExceptionSink* xsink) {
    // Check filesystem sandbox
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(archive_path, QSEC_READ | QSEC_WRITE, xsink)) {
        return nullptr;
    }

    if (qore_check_cancel(xsink, "replacing ZIP archive entries")) {
        return nullptr;
    }

    // Build sets for O(1) lookup
    std::unordered_set<std::string> delete_set;
    if (delete_names) {
        ConstListIterator li(delete_names);
        while (li.next()) {
            QoreValue v = li.getValue();
            if (v.getType() == NT_STRING) {
                delete_set.insert(v.get<const QoreStringNode>()->c_str());
            }
        }
    }

    std::unordered_set<std::string> replace_set;
    if (replacements) {
        ConstHashIterator hi(replacements);
        while (hi.next()) {
            replace_set.insert(hi.getKey());
        }
    }

    // Create temp file path
    std::string temp_path = std::string(archive_path) + ".tmp";

    // Open source for reading
    void* src_reader = mz_zip_reader_create();
    if (!src_reader) {
        xsink->raiseException("ZIP-ERROR", "failed to create zip reader");
        return nullptr;
    }

    int32_t err = mz_zip_reader_open_file(src_reader, archive_path);
    if (err != MZ_OK) {
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to open archive '%s' for reading: error %d", archive_path, err);
        return nullptr;
    }

    // Open dest for writing
    void* dst_writer = mz_zip_writer_create();
    if (!dst_writer) {
        mz_zip_reader_close(src_reader);
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to create zip writer");
        return nullptr;
    }

    err = mz_zip_writer_open_file(dst_writer, temp_path.c_str(), 0, 0);
    if (err != MZ_OK) {
        mz_zip_writer_delete(&dst_writer);
        mz_zip_reader_close(src_reader);
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to create temp archive '%s': error %d", temp_path.c_str(), err);
        return nullptr;
    }

    int64 replaced_count = 0;
    int64 deleted_count = 0;
    int64 copied_count = 0;

    // Iterate source entries
    err = mz_zip_reader_goto_first_entry(src_reader);
    while (err == MZ_OK) {
        if (qore_check_cancel(xsink, "replacing ZIP archive entries")) {
            break;
        }

        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(src_reader, &file_info);
        if (err != MZ_OK) {
            break;
        }

        std::string entry_name = file_info->filename;

        if (delete_set.count(entry_name)) {
            // Skip deleted entries
            ++deleted_count;
        } else if (replace_set.count(entry_name)) {
            // Write replacement data
            QoreValue rep_val = replacements->getKeyValue(entry_name.c_str());
            const BinaryNode* rep_data = nullptr;
            if (rep_val.getType() == NT_BINARY) {
                rep_data = rep_val.get<const BinaryNode>();
            } else if (rep_val.getType() == NT_HASH) {
                const QoreHashNode* rep_hash = rep_val.get<const QoreHashNode>();
                QoreValue data_val = rep_hash->getKeyValue("data");
                if (data_val.getType() == NT_BINARY) {
                    rep_data = data_val.get<const BinaryNode>();
                }
            }

            if (rep_data) {
                mz_zip_file new_info;
                memset(&new_info, 0, sizeof(new_info));
                new_info.filename = entry_name.c_str();
                new_info.compression_method = file_info->compression_method;
                new_info.modified_date = time(nullptr);
                new_info.uncompressed_size = rep_data->size();

                mz_zip_writer_set_compress_method(dst_writer, file_info->compression_method);

                int32_t add_err = mz_zip_writer_add_buffer(dst_writer, (void*)rep_data->getPtr(),
                                                            (int32_t)rep_data->size(), &new_info);
                if (add_err != MZ_OK) {
                    xsink->raiseException("ZIP-ERROR", "failed to write replacement for '%s': error %d",
                                          entry_name.c_str(), add_err);
                    break;
                }
                ++replaced_count;
            } else {
                xsink->raiseException("ZIP-ERROR", "replacement for '%s' must be binary data or a hash with 'data' key",
                                      entry_name.c_str());
                break;
            }
        } else {
            // Copy entry as-is (zero-copy)
            int32_t copy_err = mz_zip_writer_copy_from_reader(dst_writer, src_reader);
            if (copy_err != MZ_OK) {
                xsink->raiseException("ZIP-ERROR", "failed to copy entry '%s': error %d",
                                      entry_name.c_str(), copy_err);
                break;
            }
            ++copied_count;
        }

        err = mz_zip_reader_goto_next_entry(src_reader);
    }

    // Close both
    mz_zip_writer_close(dst_writer);
    mz_zip_writer_delete(&dst_writer);
    mz_zip_reader_close(src_reader);
    mz_zip_reader_delete(&src_reader);

    if (*xsink) {
        // Clean up temp file on error
        unlink(temp_path.c_str());
        return nullptr;
    }

    if (err != MZ_END_OF_LIST && err != MZ_OK) {
        unlink(temp_path.c_str());
        xsink->raiseException("ZIP-ERROR", "error iterating archive entries: %d", err);
        return nullptr;
    }

    // Replace original with temp
    if (rename(temp_path.c_str(), archive_path) != 0) {
        unlink(temp_path.c_str());
        xsink->raiseException("ZIP-ERROR", "failed to rename temp file to '%s': %s", archive_path, strerror(errno));
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> result(new QoreHashNode(autoTypeInfo), xsink);
    result->setKeyValue("success", true, xsink);
    result->setKeyValue("replaced_count", replaced_count, xsink);
    result->setKeyValue("deleted_count", deleted_count, xsink);
    result->setKeyValue("copied_count", copied_count, xsink);
    return result.release();
}

QoreHashNode* QoreZipFile::diff(const char* archive1, const char* archive2, ExceptionSink* xsink) {
    // Check filesystem sandbox
    QoreSandboxManagerHelper smh;
    if (smh) {
        if (!smh->checkFilesystemAccess(archive1, QSEC_READ, xsink)) {
            return nullptr;
        }
        if (!smh->checkFilesystemAccess(archive2, QSEC_READ, xsink)) {
            return nullptr;
        }
    }

    if (qore_check_cancel(xsink, "comparing ZIP archives")) {
        return nullptr;
    }

    // Read entries from both archives
    QoreZipFile zip1(archive1, ZIP_MODE_READ, xsink);
    if (*xsink) {
        return nullptr;
    }
    ReferenceHolder<QoreListNode> list1(zip1.entries(xsink), xsink);
    if (*xsink || !list1) {
        return nullptr;
    }

    QoreZipFile zip2(archive2, ZIP_MODE_READ, xsink);
    if (*xsink) {
        return nullptr;
    }
    ReferenceHolder<QoreListNode> list2(zip2.entries(xsink), xsink);
    if (*xsink || !list2) {
        return nullptr;
    }

    // Build maps: name -> (index, crc32) for each archive
    struct EntryRef {
        size_t index;
        int64 crc32;
    };
    std::unordered_map<std::string, EntryRef> map1, map2;

    for (size_t i = 0; i < list1->size(); ++i) {
        QoreValue ev = list1->retrieveEntry(i);
        if (ev.getType() == NT_HASH) {
            const QoreHashNode* h = ev.get<const QoreHashNode>();
            QoreValue nv = h->getKeyValue("name");
            if (nv.getType() == NT_STRING) {
                map1[nv.get<const QoreStringNode>()->c_str()] = {i, h->getKeyValue("crc32").getAsBigInt()};
            }
        }
    }

    for (size_t i = 0; i < list2->size(); ++i) {
        QoreValue ev = list2->retrieveEntry(i);
        if (ev.getType() == NT_HASH) {
            const QoreHashNode* h = ev.get<const QoreHashNode>();
            QoreValue nv = h->getKeyValue("name");
            if (nv.getType() == NT_STRING) {
                map2[nv.get<const QoreStringNode>()->c_str()] = {i, h->getKeyValue("crc32").getAsBigInt()};
            }
        }
    }

    // Build result lists
    ReferenceHolder<QoreListNode> added(new QoreListNode(hashdeclZipEntryInfo->getTypeInfo(true)), xsink);
    ReferenceHolder<QoreListNode> deleted(new QoreListNode(hashdeclZipEntryInfo->getTypeInfo(true)), xsink);
    ReferenceHolder<QoreListNode> modified(new QoreListNode(autoHashTypeInfo), xsink);
    ReferenceHolder<QoreListNode> unchanged(new QoreListNode(hashdeclZipEntryInfo->getTypeInfo(true)), xsink);

    // Check entries in archive1
    for (auto& p : map1) {
        auto it2 = map2.find(p.first);
        if (it2 == map2.end()) {
            deleted->push(list1->getReferencedEntry(p.second.index), xsink);
        } else if (p.second.crc32 == it2->second.crc32) {
            unchanged->push(list1->getReferencedEntry(p.second.index), xsink);
        } else {
            ReferenceHolder<QoreHashNode> mod(new QoreHashNode(autoTypeInfo), xsink);
            mod->setKeyValue("old", list1->getReferencedEntry(p.second.index), xsink);
            mod->setKeyValue("new", list2->getReferencedEntry(it2->second.index), xsink);
            modified->push(mod.release(), xsink);
        }
    }

    // Entries only in archive2 = added
    for (auto& p : map2) {
        if (map1.find(p.first) == map1.end()) {
            added->push(list2->getReferencedEntry(p.second.index), xsink);
        }
    }

    // Build result
    int64 added_count = added->size();
    int64 deleted_count = deleted->size();
    int64 modified_count = modified->size();
    int64 unchanged_count = unchanged->size();

    ReferenceHolder<QoreHashNode> result(new QoreHashNode(hashdeclZipDiffResult, xsink), xsink);
    result->setKeyValue("added", added.release(), xsink);
    result->setKeyValue("deleted", deleted.release(), xsink);
    result->setKeyValue("modified", modified.release(), xsink);
    result->setKeyValue("unchanged", unchanged.release(), xsink);
    result->setKeyValue("added_count", added_count, xsink);
    result->setKeyValue("deleted_count", deleted_count, xsink);
    result->setKeyValue("modified_count", modified_count, xsink);
    result->setKeyValue("unchanged_count", unchanged_count, xsink);
    return result.release();
}

QoreHashNode* QoreZipFile::recompress(const char* archive_path, int16_t compression_method,
                                       int16_t compression_level, const char* pwd,
                                       ExceptionSink* xsink) {
    QoreSandboxManagerHelper smh;
    if (smh && !smh->checkFilesystemAccess(archive_path, QSEC_READ | QSEC_WRITE, xsink)) {
        return nullptr;
    }

    if (qore_check_cancel(xsink, "recompressing ZIP archive")) {
        return nullptr;
    }

    std::string temp_path = std::string(archive_path) + ".tmp";

    // Open source
    void* src_reader = mz_zip_reader_create();
    if (!src_reader) {
        xsink->raiseException("ZIP-ERROR", "failed to create zip reader");
        return nullptr;
    }

    if (pwd && *pwd) {
        mz_zip_reader_set_password(src_reader, pwd);
    }

    int32_t err = mz_zip_reader_open_file(src_reader, archive_path);
    if (err != MZ_OK) {
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to open archive '%s': error %d", archive_path, err);
        return nullptr;
    }

    // Open dest
    void* dst_writer = mz_zip_writer_create();
    if (!dst_writer) {
        mz_zip_reader_close(src_reader);
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to create zip writer");
        return nullptr;
    }

    mz_zip_writer_set_compress_method(dst_writer, compression_method);
    mz_zip_writer_set_compress_level(dst_writer, compression_level);

    if (pwd && *pwd) {
        mz_zip_writer_set_password(dst_writer, pwd);
        mz_zip_writer_set_aes(dst_writer, 1);
    }

    err = mz_zip_writer_open_file(dst_writer, temp_path.c_str(), 0, 0);
    if (err != MZ_OK) {
        mz_zip_writer_delete(&dst_writer);
        mz_zip_reader_close(src_reader);
        mz_zip_reader_delete(&src_reader);
        xsink->raiseException("ZIP-ERROR", "failed to create temp archive: error %d", err);
        return nullptr;
    }

    int64 entry_count = 0;
    int64 original_size = 0;

    // Iterate and re-encode each entry
    err = mz_zip_reader_goto_first_entry(src_reader);
    while (err == MZ_OK) {
        if (qore_check_cancel(xsink, "recompressing ZIP archive entry")) {
            break;
        }

        mz_zip_file* file_info = nullptr;
        err = mz_zip_reader_entry_get_info(src_reader, &file_info);
        if (err != MZ_OK) {
            break;
        }

        original_size += file_info->compressed_size;

        // Check if directory
        size_t name_len = strlen(file_info->filename);
        bool is_dir = (name_len > 0 && file_info->filename[name_len - 1] == '/');

        if (is_dir) {
            // Copy directory entries as-is
            int32_t copy_err = mz_zip_writer_copy_from_reader(dst_writer, src_reader);
            if (copy_err != MZ_OK) {
                xsink->raiseException("ZIP-ERROR", "failed to copy directory '%s': error %d",
                                      file_info->filename, copy_err);
                break;
            }
        } else {
            // Read entry data
            int32_t open_err = mz_zip_reader_entry_open(src_reader);
            if (open_err != MZ_OK) {
                xsink->raiseException("ZIP-ERROR", "failed to open entry '%s': error %d",
                                      file_info->filename, open_err);
                break;
            }

            // Read all data into buffer
            SimpleRefHolder<BinaryNode> data(new BinaryNode());
            char buf[65536];
            while (true) {
                int32_t bytes_read = mz_zip_reader_entry_read(src_reader, buf, sizeof(buf));
                if (bytes_read == 0) {
                    break;
                }
                if (bytes_read < 0) {
                    xsink->raiseException("ZIP-ERROR", "failed to read entry '%s': error %d",
                                          file_info->filename, bytes_read);
                    break;
                }
                data->append(buf, bytes_read);
            }
            mz_zip_reader_entry_close(src_reader);

            if (*xsink) {
                break;
            }

            // Write with new compression
            mz_zip_file new_info;
            memset(&new_info, 0, sizeof(new_info));
            new_info.filename = file_info->filename;
            new_info.compression_method = compression_method;
            new_info.modified_date = file_info->modified_date;
            new_info.uncompressed_size = data->size();
            new_info.external_fa = file_info->external_fa;
            new_info.version_madeby = file_info->version_madeby;

            if (file_info->comment && file_info->comment_size > 0) {
                new_info.comment = file_info->comment;
                new_info.comment_size = file_info->comment_size;
            }

            if (pwd && *pwd) {
                new_info.aes_version = MZ_AES_VERSION;
            }

            int32_t add_err = mz_zip_writer_add_buffer(dst_writer, (void*)data->getPtr(),
                                                        (int32_t)data->size(), &new_info);
            if (add_err != MZ_OK) {
                xsink->raiseException("ZIP-ERROR", "failed to write entry '%s': error %d",
                                      file_info->filename, add_err);
                break;
            }
        }

        ++entry_count;
        err = mz_zip_reader_goto_next_entry(src_reader);
    }

    mz_zip_writer_close(dst_writer);
    mz_zip_writer_delete(&dst_writer);
    mz_zip_reader_close(src_reader);
    mz_zip_reader_delete(&src_reader);

    if (*xsink) {
        unlink(temp_path.c_str());
        return nullptr;
    }

    if (err != MZ_END_OF_LIST && err != MZ_OK) {
        unlink(temp_path.c_str());
        xsink->raiseException("ZIP-ERROR", "error iterating archive entries: %d", err);
        return nullptr;
    }

    // Get new archive size before rename
    struct stat st;
    int64 new_size = 0;
    if (stat(temp_path.c_str(), &st) == 0) {
        new_size = st.st_size;
    }

    if (rename(temp_path.c_str(), archive_path) != 0) {
        unlink(temp_path.c_str());
        xsink->raiseException("ZIP-ERROR", "failed to rename temp file: %s", strerror(errno));
        return nullptr;
    }

    ReferenceHolder<QoreHashNode> result(new QoreHashNode(autoTypeInfo), xsink);
    result->setKeyValue("success", true, xsink);
    result->setKeyValue("entry_count", entry_count, xsink);
    result->setKeyValue("original_size", original_size, xsink);
    result->setKeyValue("new_size", new_size, xsink);
    return result.release();
}

void QoreZipFile::setPassword(const char* pwd) {
    QoreAutoRWWriteLocker lock(rwlock);
    if (pwd) {
        password = pwd;
    } else {
        password.clear();
    }
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
    int encryption_method;
    parseAddOptions(opts, compression_method, compression_level, entry_password, comment, modified_time,
                     encryption_method, xsink);

    if (!entry_password.empty()) {
        mz_zip_writer_set_password(writer, entry_password.c_str());
        if (encryption_method == ZIP_EM_TRAD_PKWARE) {
            mz_zip_writer_set_aes(writer, 0);
        } else if (encryption_method != ZIP_EM_NONE) {
            mz_zip_writer_set_aes(writer, 1);
        }
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
