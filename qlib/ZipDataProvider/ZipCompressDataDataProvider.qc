# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipCompressDataDataProvider class definition

/** ZipCompressDataDataProvider.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Compress data data provider
public class ZipCompressDataDataProvider inherits AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "compress",
            "desc": "Compress data into a ZIP archive",
            "type": "ZipCompressDataDataProvider",
            "supports_request": True,
        };

        #! Request type
        static ZipCompressDataRequestDataType RequestType();

        #! Response type
        static ZipCompressDataResponseDataType ResponseType();
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
        @debug(log(LoggerLevel::DEBUG, "doRequestImpl: compressing %d entries", request.entries.size()));

        ZipFile zip();

        foreach hash<auto> entry in (request.entries) {
            hash<ZipAddOptions> add_opts();
            if (request.compression_method) {
                add_opts.compression_method = request.compression_method;
            }
            if (request.password) {
                add_opts.password = request.password;
            }

            zip.add(entry.name, entry.data, add_opts);
        }

        binary data = zip.toData();

        if (request.output_location) {
            FileLocationHandler::writeFileToLocation(request.output_location, data);
        }

        return {
            "data": data,
            "entry_count": request.entries.size(),
            "size": data.size(),
        };
    }
}
}
