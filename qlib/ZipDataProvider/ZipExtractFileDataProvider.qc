# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipExtractFileDataProvider class definition

/** ZipExtractFileDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Extract file data provider
public class ZipExtractFileDataProvider inherits AbstractDataProvider {
    public {
        const ProviderInfo = <DataProviderInfo>{
            "name": "extract",
            "desc": "Extract a single file from a ZIP archive",
            "type": "ZipExtractFileDataProvider",
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
        return AbstractDataProviderType::get(new Type("hash<ZipExtractFileRequest>"));
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipExtractFileResponse>"));
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<ZipExtractFileRequest> request = req;

        ZipFile zip;
        if (request.archive_path) {
            zip = new ZipFile(request.archive_path, "r");
        } else if (request.archive_data) {
            zip = new ZipFile(request.archive_data);
        } else {
            throw "ZIP-ERROR", "Either archive_path or archive_data must be provided";
        }

        binary data = zip.read(request.entry_name);
        zip.close();

        hash<ZipExtractFileResponse> response = <ZipExtractFileResponse>{
            "success": True,
            "entry_name": request.entry_name,
            "size": data.size(),
        };

        if (request.output_path) {
            File f();
            f.open2(request.output_path, O_CREAT | O_WRONLY | O_TRUNC);
            f.write(data);
            f.close();
            response.output_path = request.output_path;
        } else {
            response.data = data;
        }

        return response;
    }
}
}
