/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreZipFile.h QoreZipFile class header */
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

#ifndef _QORE_ZIP_QOREZIPFILE_H
#define _QORE_ZIP_QOREZIPFILE_H

#include "zip-module.h"

#include <string>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <sstream>

//! Default maximum size for memory allocations (1GB)
#define ZIP_DEFAULT_MAX_ALLOC_SIZE (1024LL * 1024 * 1024)

//! Default memory stream grow size (128KB)
#define ZIP_MEM_STREAM_GROW_SIZE (128 * 1024)

// Open modes
enum ZipMode {
    ZIP_MODE_READ = 0,
    ZIP_MODE_WRITE = 1,
    ZIP_MODE_APPEND = 2,
    ZIP_MODE_MEMORY = 3
};

//! QoreZipFile - private data class for ZipFile Qore class
/** This class is thread-safe. All public methods acquire appropriate locks.
    However, stream objects (ZipInputStream, ZipOutputStream) are not thread-safe
    and should only be used from a single thread.
*/
class QoreZipFile : public AbstractPrivateData {
public:
    //! Constructor for file-based archive
    DLLLOCAL QoreZipFile(const char* path, ZipMode mode, ExceptionSink* xsink, bool recover = false);

    //! Constructor for in-memory archive (from binary data)
    DLLLOCAL QoreZipFile(const BinaryNode* data, ExceptionSink* xsink, bool recover = false);

    //! Constructor for new in-memory archive
    DLLLOCAL QoreZipFile(ExceptionSink* xsink);

    //! Increment the active stream count
    DLLLOCAL void refStream() { ++active_streams; }

    //! Decrement the active stream count
    DLLLOCAL void derefStream() { --active_streams; }

    //! Check if there are active streams
    DLLLOCAL bool hasActiveStreams() const { return active_streams > 0; }

    //! Get the maximum allocation size
    DLLLOCAL int64 getMaxAllocSize() const { return max_alloc_size; }

    //! Set the maximum allocation size for memory allocations
    DLLLOCAL void setMaxAllocSize(int64 size) { max_alloc_size = size; }

    //! Destructor
    DLLLOCAL virtual ~QoreZipFile();

    //! Close the archive
    DLLLOCAL void close(ExceptionSink* xsink);

    //! Get archive as binary data (for in-memory archives)
    DLLLOCAL BinaryNode* toData(ExceptionSink* xsink);

    //! Get list of all entries
    DLLLOCAL QoreListNode* entries(ExceptionSink* xsink);

    //! Get number of entries
    DLLLOCAL int64 count(ExceptionSink* xsink);

    //! Check if entry exists
    DLLLOCAL bool hasEntry(const char* name, ExceptionSink* xsink);

    //! Read entry as binary data
    DLLLOCAL BinaryNode* read(const char* name, ExceptionSink* xsink);

    //! Read entry as text
    DLLLOCAL QoreStringNode* readText(const char* name, const char* encoding, ExceptionSink* xsink);

    //! Get entry info
    DLLLOCAL QoreHashNode* getEntry(const char* name, ExceptionSink* xsink);

    //! Add binary data as entry
    DLLLOCAL void add(const char* name, const BinaryNode* data, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Add text as entry
    DLLLOCAL void addText(const char* name, const QoreStringNode* text, const char* encoding,
                          const QoreHashNode* opts, ExceptionSink* xsink);

    //! Add file from filesystem
    DLLLOCAL void addFile(const char* name, const char* filepath, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Add directory entry
    DLLLOCAL void addDirectory(const char* name, ExceptionSink* xsink);

    //! Add symlink entry
    DLLLOCAL void addSymlink(const char* name, const char* target, ExceptionSink* xsink);

    //! Verify archive integrity by reading all entries and checking CRC
    DLLLOCAL QoreListNode* verify(const QoreHashNode* opts, ExceptionSink* xsink);

    //! Extract all entries to directory
    DLLLOCAL void extractAll(const char* destPath, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Extract specific entries to directory; returns list of extracted file paths
    DLLLOCAL QoreListNode* extractEntries(const char* destPath, const QoreListNode* entryNames,
                                          const QoreHashNode* opts, ExceptionSink* xsink);

    //! Extract single entry
    DLLLOCAL void extractEntry(const char* name, const char* destPath, ExceptionSink* xsink);

    //! Add a filesystem path (file or directory) recursively to the archive
    DLLLOCAL void addPath(const char* path, const char* root_path, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Delete entry (not supported)
    DLLLOCAL void deleteEntry(const char* name, ExceptionSink* xsink);

    //! Replace or delete entries in an archive (static — operates on closed archive files)
    DLLLOCAL static QoreHashNode* replaceEntries(const char* archive_path, const QoreHashNode* replacements,
                                                  const QoreListNode* delete_names, ExceptionSink* xsink);

    //! Compare two archives and return differences
    DLLLOCAL static QoreHashNode* diff(const char* archive1, const char* archive2, ExceptionSink* xsink);

    //! Re-compress an archive with a different compression method
    DLLLOCAL static QoreHashNode* recompress(const char* archive_path, int16_t compression_method,
                                              int16_t compression_level, const char* pwd,
                                              ExceptionSink* xsink);

    //! Set password for reading encrypted entries
    DLLLOCAL void setPassword(const char* pwd);

    //! Get archive path
    DLLLOCAL QoreStringNode* getPath() const;

    //! Get archive comment
    DLLLOCAL QoreStringNode* getComment(ExceptionSink* xsink);

    //! Set archive comment
    DLLLOCAL void setComment(const char* comment, ExceptionSink* xsink);

    //! Open an input stream for reading an entry
    DLLLOCAL QoreObject* openInputStream(const char* name, ExceptionSink* xsink);

    //! Open an output stream for writing an entry
    DLLLOCAL QoreObject* openOutputStream(const char* name, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Get reader handle (for stream classes)
    DLLLOCAL void* getReader() const { return reader; }

    //! Get writer handle (for stream classes)
    DLLLOCAL void* getWriter() const { return writer; }

private:
    mutable QoreRWLock rwlock;          //!< Read-write lock for thread safety
    std::string filepath;
    ZipMode mode;
    void* reader;                        //!< mz_zip_reader handle
    void* writer;                        //!< mz_zip_writer handle
    void* mem_stream;                    //!< memory stream for in-memory archives
    std::string password;
    bool in_memory;
    bool closed;
    bool recover;                    //!< Recovery mode for corrupted archives
    std::atomic<int> active_streams;     //!< Count of active stream objects
    int64 max_alloc_size;                //!< Maximum size for memory allocations

    //! Create ZipEntryInfo hash from minizip file info
    DLLLOCAL QoreHashNode* createEntryInfo(mz_zip_file* file_info, ExceptionSink* xsink);

    //! Parse add options
    DLLLOCAL void parseAddOptions(const QoreHashNode* opts, int16_t& compression_method, int16_t& compression_level,
                                  std::string& entry_password, std::string& comment, int64& modified_time,
                                  int& encryption_method, ExceptionSink* xsink);

    //! Check archive is open and in correct mode (must be called with lock held)
    DLLLOCAL bool checkOpenUnlocked(ExceptionSink* xsink, bool forWrite = false);

    //! Open for reading
    DLLLOCAL void openRead(ExceptionSink* xsink);

    //! Open for writing
    DLLLOCAL void openWrite(ExceptionSink* xsink);

    //! Validate path for extraction (check for path traversal)
    DLLLOCAL static bool validateExtractPath(const char* entry_name, const char* dest_path, ExceptionSink* xsink);

    //! Validate a symlink target stays within the destination directory
    DLLLOCAL static bool validateSymlink(const char* entry_name, const char* link_target,
                                          const char* dest_path, ExceptionSink* xsink);

    //! Check if the current reader entry is a symlink
    DLLLOCAL bool isSymlinkEntry();

    //! Get the symlink target for the current entry (from linkname or entry data)
    DLLLOCAL std::string getSymlinkTarget(ExceptionSink* xsink);

    //! Normalize a path by collapsing . and .. components (no filesystem access)
    DLLLOCAL static std::string normalizePath(const std::string& path);

    //! Add binary data as entry (must be called with write lock held)
    DLLLOCAL void addUnlocked(const char* name, const BinaryNode* data, const QoreHashNode* opts, ExceptionSink* xsink);

    //! Parse extract options from QoreHashNode
    DLLLOCAL void parseExtractOptions(const QoreHashNode* opts, std::string& pwd, bool& overwrite,
                                      bool& preserve_paths, bool& allow_symlinks,
                                      std::string& strip_prefix, std::string& add_prefix,
                                      ResolvedCallReferenceNode*& entry_callback,
                                      ExceptionSink* xsink);

    //! Set up overwrite and entry callbacks on the reader
    DLLLOCAL void setupCallbacks(struct QoreExtractCallbackData& cb_data);

    //! Clear all callbacks on the reader
    DLLLOCAL void clearCallbacks();

    //! Extract a single entry to a constructed path (must be called with lock held and entry positioned)
    DLLLOCAL int32_t extractCurrentEntry(const char* destPath, const char* entry_name, bool preserve_paths,
                                          const std::string& strip_prefix, const std::string& add_prefix,
                                          std::string& out_path, ExceptionSink* xsink);
};

//! QoreZipEntry - private data class for ZipEntry Qore class
class QoreZipEntry : public AbstractPrivateData {
public:
    DLLLOCAL QoreZipEntry(const std::string& name, int64 size, int64 compressed_size,
                          int64 modified, int64 crc32, int compression_method,
                          bool is_dir, bool is_encrypted, const std::string& comment);

    DLLLOCAL virtual ~QoreZipEntry();

    DLLLOCAL QoreStringNode* getName() const;
    DLLLOCAL int64 getSize() const { return size; }
    DLLLOCAL int64 getCompressedSize() const { return compressed_size; }
    DLLLOCAL DateTimeNode* getModified() const;
    DLLLOCAL int64 getCrc32() const { return crc32; }
    DLLLOCAL int getCompressionMethod() const { return compression_method; }
    DLLLOCAL bool isDirectory() const { return is_dir; }
    DLLLOCAL bool isEncrypted() const { return is_encrypted; }
    DLLLOCAL QoreStringNode* getComment() const;

private:
    std::string name;
    int64 size;
    int64 compressed_size;
    int64 modified;
    int64 crc32;
    int compression_method;
    bool is_dir;
    bool is_encrypted;
    std::string comment;
};

#endif // _QORE_ZIP_QOREZIPFILE_H
