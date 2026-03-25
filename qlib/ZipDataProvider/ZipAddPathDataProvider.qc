# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipAddPathDataProvider class definition

/** ZipAddPathDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Add path data provider
public class ZipAddPathDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "add-path",
            "desc": "Recursively add a file or directory to a ZIP archive",
            "type": "ZipAddPathDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipAddPathRequestDataType RequestType();

        #! Response type
        static ZipAddPathResponseDataType ResponseType();
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
        return RequestType;
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return ResponseType;
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<auto> request = req;
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: adding path %y to archive %y",
            request.source_path, request.archive_path));

        ZipFile zip(request.archive_path, "a");

        hash<ZipAddOptions> add_opts();
        if (request.compression_method) {
            add_opts.compression_method = request.compression_method;
        }
        if (request.password) {
            add_opts.password = request.password;
        }
        if (request.encryption_method) {
            add_opts.encryption_method = request.encryption_method;
        }

        zip.addPath(request.source_path, request.root_path, add_opts);
        zip.close();

        return {
            "success": True,
        };
    }
}
}
