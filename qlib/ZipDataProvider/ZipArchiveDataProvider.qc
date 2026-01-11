# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipArchiveDataProvider class definition

/** ZipArchiveDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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

#! Contains all public definitions in the ZipDataProvider module
public namespace ZipDataProvider {
#! Archive operations provider
public class ZipArchiveDataProvider inherits AbstractDataProvider {
    public {
        const ProviderInfo = <DataProviderInfo>{
            "name": "archive",
            "desc": "ZIP archive operations",
            "type": "ZipArchiveDataProvider",
            "supports_children": True,
        };
    }

    private {
        *hash<auto> opts;
    }

    constructor(*hash<auto> opts) {
        self.opts = opts;
    }

    string getName() {
        return ProviderInfo.name;
    }

    private *list<string> getChildProviderNamesImpl() {
        return ("create", "extract", "list", "info", "add");
    }

    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "create":
                return new ZipCreateArchiveDataProvider(opts);
            case "extract":
                return new ZipExtractArchiveDataProvider(opts);
            case "list":
                return new ZipListArchiveDataProvider(opts);
            case "info":
                return new ZipArchiveInfoDataProvider(opts);
            case "add":
                return new ZipAddFilesDataProvider(opts);
        }
        return NOTHING;
    }

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }
}
}
