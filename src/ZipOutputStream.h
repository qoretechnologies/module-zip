/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file ZipOutputStream.h ZipOutputStream class header */
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

#ifndef _QORE_ZIP_ZIPOUTPUTSTREAM_H
#define _QORE_ZIP_ZIPOUTPUTSTREAM_H

#include "zip-module.h"
#include <qore/OutputStream.h>

#include <string>

//! ZipOutputStream - OutputStream for writing a single entry to a ZIP archive
class ZipOutputStream : public OutputStream {
public:
    //! Constructor - opens entry for writing
    /** @param writer the minizip writer handle
        @param entry_name the name of the entry being written
        @param compression_method compression method to use
        @param compression_level compression level (0-9)
        @param xsink exception sink
    */
    DLLLOCAL ZipOutputStream(void* writer, const std::string& entry_name,
                              int16_t compression_method, int16_t compression_level,
                              ExceptionSink* xsink);

    //! Destructor
    DLLLOCAL virtual ~ZipOutputStream();

    //! Returns the name of the class
    DLLLOCAL virtual const char* getName() override {
        return "ZipOutputStream";
    }

    //! Returns true if the stream has been closed
    DLLLOCAL virtual bool isClosed() override {
        return closed;
    }

    //! Closes the stream
    /** @param xsink exception sink
    */
    DLLLOCAL virtual void close(ExceptionSink* xsink) override;

    //! Writes bytes to the output stream
    /** @param ptr the source buffer
        @param count the number of bytes to write
        @param xsink exception sink
    */
    DLLLOCAL virtual void write(const void* ptr, int64 count, ExceptionSink* xsink) override;

private:
    void* writer;           //!< minizip writer handle (not owned)
    std::string entry_name; //!< name of the entry being written
    bool entry_open;        //!< true if entry is currently open
    bool closed;            //!< true if stream has been closed
};

#endif // _QORE_ZIP_ZIPOUTPUTSTREAM_H
