/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file ZipInputStream.h ZipInputStream class header */
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

#ifndef _QORE_ZIP_ZIPINPUTSTREAM_H
#define _QORE_ZIP_ZIPINPUTSTREAM_H

#include "zip-module.h"
#include <qore/InputStream.h>

#include <string>

// Forward declaration
class QoreZipFile;

//! ZipInputStream - InputStream for reading a single entry from a ZIP archive
/** @note This class is not thread-safe. Only one thread should access
    an instance at a time.
*/
class ZipInputStream : public InputStream {
public:
    //! Constructor - opens entry for reading
    /** @param parent the parent ZipFile object
        @param reader the minizip reader handle (must have entry located)
        @param entry_name the name of the entry being read
        @param xsink exception sink
    */
    DLLLOCAL ZipInputStream(QoreZipFile* parent, void* reader, const std::string& entry_name, ExceptionSink* xsink);

    //! Destructor
    DLLLOCAL virtual ~ZipInputStream();

    //! Returns the name of the class
    DLLLOCAL virtual const char* getName() override {
        return "ZipInputStream";
    }

    //! Reads up to limit bytes from the input stream
    /** @param ptr the destination buffer to read data into
        @param limit the maximum number of bytes to read
        @param xsink exception sink
        @return the number of bytes read, 0 indicates end of stream
    */
    DLLLOCAL virtual int64 read(void* ptr, int64 limit, ExceptionSink* xsink) override;

    //! Peeks the next byte from the input stream
    /** @param xsink exception sink
        @return the next byte available, -1 indicates end of stream
    */
    DLLLOCAL virtual int64 peek(ExceptionSink* xsink) override;

private:
    QoreZipFile* parent;    //!< parent ZipFile object (not owned, for reference counting)
    void* reader;           //!< minizip reader handle (not owned)
    std::string entry_name; //!< name of the entry being read
    bool entry_open;        //!< true if entry is currently open
    bool eof;               //!< true if end of entry reached
    int peek_byte;          //!< buffered peek byte, -2 if none
};

#endif // _QORE_ZIP_ZIPINPUTSTREAM_H
