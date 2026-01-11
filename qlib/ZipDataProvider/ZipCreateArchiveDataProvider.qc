# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipCreateArchiveDataProvider class definition

/** ZipCreateArchiveDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Create archive data provider
public class ZipCreateArchiveDataProvider inherits AbstractDataProvider {
    public {
        const ProviderInfo = <DataProviderInfo>{
            "name": "create",
            "desc": "Create a new ZIP archive",
            "type": "ZipCreateArchiveDataProvider",
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
        return AbstractDataProviderType::get(new Type("hash<ZipCreateArchiveRequest>"));
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipCreateArchiveResponse>"));
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<ZipCreateArchiveRequest> request = req;

        ZipFile zip;
        if (request.output_path) {
            zip = new ZipFile(request.output_path, "w");
        } else {
            zip = new ZipFile();
        }

        if (request.comment) {
            zip.setComment(request.comment);
        }

        int entry_count = 0;
        foreach hash<auto> entry in (request.entries) {
            hash<ZipAddOptions> add_opts();
            if (request.compression_method) {
                add_opts.compression_method = request.compression_method;
            }
            if (request.password) {
                add_opts.password = request.password;
            }

            if (entry.data) {
                zip.add(entry.name, entry.data, add_opts);
            } else if (entry.path) {
                zip.addFile(entry.name ?? basename(entry.path), entry.path, add_opts);
            }
            ++entry_count;
        }

        hash<ZipCreateArchiveResponse> response();
        response.success = True;
        response.entry_count = entry_count;

        if (request.output_path) {
            zip.close();
            response.output_path = request.output_path;
            response.size = hstat(request.output_path).size;
        } else {
            response.data = zip.toData();
            response.size = response.data.size();
        }

        return response;
    }
}
}
