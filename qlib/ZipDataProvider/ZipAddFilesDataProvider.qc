# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipAddFilesDataProvider class definition

/** ZipAddFilesDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Add files data provider
public class ZipAddFilesDataProvider inherits AbstractDataProvider {
    public {
        const ProviderInfo = <DataProviderInfo>{
            "name": "add",
            "desc": "Add files to an existing ZIP archive",
            "type": "ZipAddFilesDataProvider",
            "supports_request": True,
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

    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }

    private *AbstractDataProviderType getRequestTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipAddFilesRequest>"));
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipAddFilesResponse>"));
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<ZipAddFilesRequest> request = req;

        ZipFile zip(request.archive_path, "a");

        int entries_added = 0;
        foreach hash<auto> entry in (request.entries) {
            hash<ZipAddOptions> add_opts();
            if (request.password) {
                add_opts.password = request.password;
            }

            if (entry.data) {
                zip.add(entry.name, entry.data, add_opts);
            } else if (entry.path) {
                zip.addFile(entry.name ?? basename(entry.path), entry.path, add_opts);
            }
            ++entries_added;
        }

        zip.close();

        return <ZipAddFilesResponse>{
            "success": True,
            "entries_added": entries_added,
        };
    }
}
}
