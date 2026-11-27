# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipListArchiveDataProvider class definition

/** ZipListArchiveDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! List archive data provider
public class ZipListArchiveDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "list",
            "desc": "List contents of a ZIP archive",
            "type": "ZipListArchiveDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipListArchiveRequestDataType RequestType();

        #! Response type
        static ZipListArchiveResponseDataType ResponseType();
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
        # the paths and locations in the request are the caller's input, also for the code that a host-created
        # provider was lent to: refused before anything is read or written if that code is restricted (issue #7)
        zip_check_caller_path("input_path", request.input_path);
        zip_check_caller_location("location", request.location);
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: listing archive contents"));

        *binary archive_data = request.data;
        if (!archive_data && request.location) {
            archive_data = FileLocationHandler::getBinaryFileFromLocation(request.location);
        }

        ZipFile zip;
        if (request.input_path) {
            zip = new ZipFile(request.input_path, "r", request.recover);
        } else if (archive_data) {
            zip = new ZipFile(archive_data, request.recover);
        } else {
            throw "ZIP-ERROR", "one of 'input_path', 'data', or 'location' must be provided";
        }

        list<hash<ZipEntryInfo>> entries = zip.entries();
        zip.close();

        # Apply filters if any
        entries = filter_entries(entries, cast<*list<string>>(request.include_patterns),
            cast<*list<string>>(request.exclude_patterns),
            request.include_regex, request.exclude_regex);

        return {
            "entries": entries,
            "count": entries.size(),
        };
    }
}
}
