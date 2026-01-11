# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipExtractArchiveDataProvider class definition

/** ZipExtractArchiveDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Extract archive data provider
public class ZipExtractArchiveDataProvider inherits AbstractDataProvider {
    public {
        const ProviderInfo = <DataProviderInfo>{
            "name": "extract",
            "desc": "Extract a ZIP archive to a directory",
            "type": "ZipExtractArchiveDataProvider",
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
        return AbstractDataProviderType::get(new Type("hash<ZipExtractArchiveRequest>"));
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipExtractArchiveResponse>"));
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<ZipExtractArchiveRequest> request = req;

        ZipFile zip;
        if (request.input_path) {
            zip = new ZipFile(request.input_path, "r");
        } else if (request.data) {
            zip = new ZipFile(request.data);
        } else {
            throw "ZIP-ERROR", "Either input_path or data must be provided";
        }

        hash<ZipExtractOptions> extract_opts();
        if (request.password) {
            extract_opts.password = request.password;
        }

        # Get list of entries before extraction
        list<hash<ZipEntryInfo>> entries = zip.entries();
        list<string> extracted_files = ();
        foreach hash<ZipEntryInfo> entry in (entries) {
            if (!entry.is_directory) {
                extracted_files += request.destination + "/" + entry.name;
            }
        }

        zip.extractAll(request.destination, extract_opts);
        zip.close();

        return <ZipExtractArchiveResponse>{
            "success": True,
            "entry_count": entries.size(),
            "destination": request.destination,
            "extracted_files": extracted_files,
        };
    }
}
}
