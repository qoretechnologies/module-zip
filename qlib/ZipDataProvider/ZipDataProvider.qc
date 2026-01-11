# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipDataProvider class definition

/** ZipDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Main ZIP data provider class
public class ZipDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "zip",
            "desc": "ZIP archive data provider",
            "type": "ZipDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
        };
    }

    private {
        #! Options passed to constructor
        *hash<auto> opts;
    }

    #! Constructor
    constructor(*hash<auto> opts) {
        self.opts = opts;
    }

    #! Returns the data provider name
    string getName() {
        return ProviderInfo.name;
    }

    #! Returns the data provider description
    *string getDesc() {
        return ProviderInfo.desc;
    }

    #! Returns child providers
    private *list<string> getChildProviderNamesImpl() {
        return ("archive", "file", "data");
    }

    #! Returns the given child provider or NOTHING if it doesn't exist
    private *AbstractDataProvider getChildProviderImpl(string name) {
        switch (name) {
            case "archive":
                return new ZipArchiveDataProvider(opts);
            case "file":
                return new ZipFileDataProvider(opts);
            case "data":
                return new ZipDataDataProvider(opts);
        }
        return NOTHING;
    }

    #! Returns data provider static info
    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }
}
}
