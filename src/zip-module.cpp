/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file zip-module.cpp zip module implementation */
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

#include "zip-module.h"
#include "QC_ZipFile.h"
#include "QC_ZipInputStream.h"
#include "QC_ZipOutputStream.h"

static QoreStringNode* zip_module_init();
static void zip_module_ns_init(QoreNamespace* rns, QoreNamespace* qns);
static void zip_module_delete();

DLLEXPORT char qore_module_name[] = "zip";
DLLEXPORT char qore_module_version[] = "1.0.0";
DLLEXPORT char qore_module_description[] = "Qore ZIP archive module";
DLLEXPORT char qore_module_author[] = "Qore Technologies, s.r.o.";
DLLEXPORT char qore_module_url[] = "https://github.com/qoretechnologies/module-zip";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = zip_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = zip_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = zip_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_MIT;
DLLEXPORT char qore_module_license_str[] = "MIT";

// Global hashdecl pointers
const TypedHashDecl* hashdeclZipEntryInfo = nullptr;
const TypedHashDecl* hashdeclZipAddOptions = nullptr;
const TypedHashDecl* hashdeclZipExtractOptions = nullptr;

QoreNamespace ZipNS("Zip");

static QoreStringNode* zip_module_init() {
    // Initialize hashdecls (defined in QPP files for documentation)
    hashdeclZipEntryInfo = init_hashdecl_ZipEntryInfo(ZipNS);
    hashdeclZipAddOptions = init_hashdecl_ZipAddOptions(ZipNS);
    hashdeclZipExtractOptions = init_hashdecl_ZipExtractOptions(ZipNS);

    // Initialize classes - stream classes must be initialized before ZipFile
    // because ZipFile references them as return types
    ZipNS.addSystemClass(initZipInputStreamClass(ZipNS));
    ZipNS.addSystemClass(initZipOutputStreamClass(ZipNS));
    ZipNS.addSystemClass(initZipFileClass(ZipNS));
    ZipNS.addSystemClass(initZipEntryClass(ZipNS));

    return nullptr;
}

static void zip_module_ns_init(QoreNamespace* rns, QoreNamespace* qns) {
    qns->addNamespace(ZipNS.copy());
}

static void zip_module_delete() {
    // Cleanup if needed
}
