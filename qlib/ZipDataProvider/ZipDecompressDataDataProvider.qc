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
        const ProviderInfo = <DataProviderInfo>{
            "name": "decompress",
            "desc": "Decompress a ZIP archive from binary data",
            "type": "ZipDecompressDataDataProvider",
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
        return AbstractDataProviderType::get(new Type("hash<ZipDecompressDataRequest>"));
    }

    private *AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderType::get(new Type("hash<ZipDecompressDataResponse>"));
    }

    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        hash<ZipDecompressDataRequest> request = req;

        ZipFile zip(request.data);

        hash<string, binary> entries();
        list<hash<ZipEntryInfo>> entry_list = zip.entries();

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

        return <ZipDecompressDataResponse>{
            "entries": entries,
            "entry_count": entries.size(),
        };
    }
}
}
