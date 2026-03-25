# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipVerifyArchiveDataProvider class definition

/** ZipVerifyArchiveDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Verify archive data provider
public class ZipVerifyArchiveDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "verify",
            "desc": "Verify the integrity of a ZIP archive by checking CRC checksums",
            "type": "ZipVerifyArchiveDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipVerifyArchiveRequestDataType RequestType();

        #! Response type
        static ZipVerifyArchiveResponseDataType ResponseType();
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
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: verifying archive integrity"));

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

        hash<ZipExtractOptions> verify_opts();
        if (request.password) {
            verify_opts.password = request.password;
        }

        list<hash<ZipVerifyResult>> all_results = zip.verify(verify_opts);
        zip.close();

        # Apply filters if any
        if (request.include_patterns || request.exclude_patterns
                || request.include_regex || request.exclude_regex) {
            # Build fake entry list for filtering
            list<hash<ZipEntryInfo>> fake = map <ZipEntryInfo>{"name": $1.name}, all_results;
            list<hash<ZipEntryInfo>> filtered = filter_entries(fake,
                cast<*list<string>>(request.include_patterns),
                cast<*list<string>>(request.exclude_patterns),
                request.include_regex, request.exclude_regex);
            hash<string, bool> keep;
            foreach hash<ZipEntryInfo> e in (filtered) {
                keep{e.name} = True;
            }
            all_results = select all_results, keep{$1.name};
        }

        int failed = 0;
        foreach hash<ZipVerifyResult> r in (all_results) {
            if (!r.valid) {
                ++failed;
            }
        }

        return {
            "valid": failed == 0,
            "verified_count": all_results.size(),
            "failed_count": failed,
            "results": all_results,
        };
    }
}
}
