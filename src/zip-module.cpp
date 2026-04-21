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

static void zip_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink);
static void zip_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink);
static void zip_module_delete();

extern "C" DLLEXPORT void zip_qore_module_desc(QoreModuleInfo& mod_info) {
    mod_info.name = "zip";
    mod_info.version = "1.0.0";
    mod_info.desc = "Qore ZIP archive module";
    mod_info.author = "Qore Technologies, s.r.o.";
    mod_info.url = "https://github.com/qoretechnologies/module-zip";
    mod_info.api_major = QORE_MODULE_API_MAJOR;
    mod_info.api_minor = QORE_MODULE_API_MINOR;
    mod_info.init = zip_module_init;
    mod_info.ns_init = zip_module_ns_init;
    mod_info.del = zip_module_delete;
    mod_info.license = QL_MIT;
    mod_info.license_str = "MIT";
}

// Global hashdecl pointers
const TypedHashDecl* hashdeclZipEntryInfo = nullptr;
const TypedHashDecl* hashdeclZipAddOptions = nullptr;
const TypedHashDecl* hashdeclZipVerifyResult = nullptr;
const TypedHashDecl* hashdeclZipDiffResult = nullptr;
const TypedHashDecl* hashdeclZipExtractOptions = nullptr;
const TypedHashDecl* hashdeclZipExtractResult = nullptr;

QoreNamespace ZipNs("Qore::Zip");

static void zip_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink) {
    // Initialize hashdecls (defined in QPP files for documentation)
    hashdeclZipEntryInfo = init_hashdecl_ZipEntryInfo(ZipNs);
    hashdeclZipAddOptions = init_hashdecl_ZipAddOptions(ZipNs);
    hashdeclZipVerifyResult = init_hashdecl_ZipVerifyResult(ZipNs);
    hashdeclZipDiffResult = init_hashdecl_ZipDiffResult(ZipNs);
    hashdeclZipExtractOptions = init_hashdecl_ZipExtractOptions(ZipNs);
    hashdeclZipExtractResult = init_hashdecl_ZipExtractResult(ZipNs);

    // Initialize classes - stream classes must be initialized before ZipFile
    // because ZipFile references them as return types
    ZipNs.addSystemClass(initZipInputStreamClass(ZipNs));
    ZipNs.addSystemClass(initZipOutputStreamClass(ZipNs));
    ZipNs.addSystemClass(initZipFileClass(ZipNs));
    ZipNs.addSystemClass(initZipEntryClass(ZipNs));

}

static void zip_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink) {
    qns->addNamespace(ZipNs.copy());
}

static void zip_module_delete() {
    // Cleanup if needed
}
