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
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "extract",
            "desc": "Extract a ZIP archive to a directory",
            "type": "ZipExtractArchiveDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipExtractArchiveRequestDataType RequestType();

        #! Response type
        static ZipExtractArchiveResponseDataType ResponseType();
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
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: extracting archive to %y", request.destination));

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

        hash<ZipExtractOptions> extract_opts();
        if (request.password) {
            extract_opts.password = request.password;
        }
        if (request.allow_symlinks) {
            extract_opts.allow_symlinks = request.allow_symlinks;
        }
        if (request.overwrite) {
            extract_opts.overwrite = request.overwrite;
        }
        if (exists request.preserve_paths) {
            extract_opts.preserve_paths = request.preserve_paths;
        }
        if (request.strip_prefix) {
            extract_opts.strip_prefix = request.strip_prefix;
        }
        if (request.add_prefix) {
            extract_opts.add_prefix = request.add_prefix;
        }

        # Get list of entries before extraction
        list<hash<ZipEntryInfo>> all_entries = zip.entries();

        # Apply filters if any
        bool has_filters = request.include_patterns || request.exclude_patterns
            || request.include_regex || request.exclude_regex;

        if (has_filters) {
            list<hash<ZipEntryInfo>> filtered = filter_entries(all_entries,
                cast<*list<string>>(request.include_patterns),
                cast<*list<string>>(request.exclude_patterns),
                request.include_regex, request.exclude_regex);

            # Build lists of filtered names and skipped names
            list<string> filtered_names = map $1.name, filtered;
            list<string> extracted_files = zip.extractEntries(request.destination, filtered_names, extract_opts);

            # Compute skipped entries
            hash<string, bool> filtered_set;
            foreach string name in (filtered_names) {
                filtered_set{name} = True;
            }
            list<string> skipped_files = ();
            foreach hash<ZipEntryInfo> entry in (all_entries) {
                if (!entry.is_directory && !filtered_set{entry.name}) {
                    skipped_files += entry.name;
                }
            }

            zip.close();

            return {
                "success": True,
                "entry_count": filtered.size(),
                "destination": request.destination,
                "extracted_files": extracted_files,
                "skipped_count": skipped_files.size(),
                "skipped_files": skipped_files,
            };
        }

        # No filters: extract all. Use the ZipExtractResult to report accurate counts, not the
        # archive listing — skipped files must not be counted as extracted.
        hash<ZipExtractResult> result = zip.extractAll(request.destination, extract_opts);
        zip.close();

        return {
            "success": True,
            "entry_count": result.extracted_count,
            "destination": request.destination,
            "extracted_files": result.extracted,
            "skipped_files": result.skipped,
        };
    }
}
}
