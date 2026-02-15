# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipArchiveInfoDataProvider class definition

/** ZipArchiveInfoDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Archive info data provider
public class ZipArchiveInfoDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "info",
            "desc": "Get ZIP archive metadata",
            "type": "ZipArchiveInfoDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipArchiveInfoRequestDataType RequestType();

        #! Response type
        static ZipArchiveInfoResponseDataType ResponseType();
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
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: getting archive info"));

        *binary archive_data = request.data;
        if (!archive_data && request.location) {
            archive_data = FileLocationHandler::getBinaryFileFromLocation(request.location);
        }

        ZipFile zip;
        if (request.input_path) {
            zip = new ZipFile(request.input_path, "r");
        } else if (archive_data) {
            zip = new ZipFile(archive_data);
        } else {
            throw "ZIP-ERROR", "one of 'input_path', 'data', or 'location' must be provided";
        }

        list<hash<ZipEntryInfo>> entries = zip.entries();
        int total_size = 0;
        int compressed_size = 0;
        foreach hash<ZipEntryInfo> entry in (entries) {
            total_size += entry.size;
            compressed_size += entry.compressed_size;
        }

        hash<auto> response = {
            "path": request.input_path,
            "entry_count": entries.size(),
            "total_size": total_size,
            "compressed_size": compressed_size,
            "comment": zip.comment(),
        };

        zip.close();
        return response;
    }
}
}
