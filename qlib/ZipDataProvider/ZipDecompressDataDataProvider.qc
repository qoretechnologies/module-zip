# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipDecompressDataDataProvider class definition

/** ZipDecompressDataDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Decompress data data provider
public class ZipDecompressDataDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "decompress",
            "desc": "Decompress a ZIP archive from binary data",
            "type": "ZipDecompressDataDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipDecompressDataRequestDataType RequestType();

        #! Response type
        static ZipDecompressDataResponseDataType ResponseType();
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

        *binary archive_data = request.data;
        if (!archive_data && request.archive_location) {
            archive_data = FileLocationHandler::getBinaryFileFromLocation(request.archive_location);
        }

        ZipFile zip;
        if (request.archive_path) {
            zip = new ZipFile(request.archive_path, "r", request.recover);
        } else if (archive_data) {
            zip = new ZipFile(archive_data, request.recover);
        } else {
            throw "ZIP-ERROR", "one of 'data', 'archive_path', or 'archive_location' must be provided";
        }

        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: decompressing archive"));

        list<hash<ZipEntryInfo>> entry_list = zip.entries();

        # Apply pattern/regex filters
        entry_list = filter_entries(entry_list, cast<*list<string>>(request.include_patterns),
            cast<*list<string>>(request.exclude_patterns),
            request.include_regex, request.exclude_regex);

        hash<string, binary> entries();
        foreach hash<ZipEntryInfo> entry in (entry_list) {
            if (entry.is_directory) {
                continue;
            }

            # If specific entry names are requested, only extract those
            if (request.entry_names && request.entry_names.size() > 0) {
                if (!inlist(entry.name, request.entry_names)) {
                    continue;
                }
            }

            entries{entry.name} = zip.read(entry.name);
        }

        zip.close();

        return {
            "entries": entries,
            "entry_count": entries.size(),
        };
    }
}
}
