# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipDataProvider enum definitions

/** ZipDataProviderEnums.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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

#! Compression methods for ZIP archives
/** These values correspond to the ZIP specification compression method codes.

    @see @ref zip_compression_methods for C++ constant definitions
*/
public enum CompressionMethod : int {
    #! No compression (store only)
    Store = 0,
    #! Deflate compression (standard, most compatible)
    Deflate = 8,
    #! BZip2 compression
    BZip2 = 12,
    #! LZMA compression
    Lzma = 14,
    #! Zstandard compression (fast, high ratio)
    Zstd = 93,
    #! XZ/LZMA2 compression
    Xz = 95,
}

#! Compression levels for ZIP archives
/** These values control the compression effort/quality tradeoff.

    @see @ref zip_compression_levels for C++ constant definitions
*/
public enum CompressionLevel : int {
    #! Default compression level
    Default = -1,
    #! No compression (store only)
    Store = 0,
    #! Fastest compression (least compression)
    Fastest = 1,
    #! Best (maximum) compression
    Best = 9,
}

#! Encryption methods for ZIP archives
/** These values specify the encryption algorithm to use.

    @see @ref zip_encryption_methods for C++ constant definitions
*/
public enum EncryptionMethod : int {
    #! No encryption
    None = 0,
    #! Traditional PKWARE encryption (weak, not recommended)
    TradPkware = 1,
    #! AES-128 encryption
    Aes128 = 0x0101,
    #! AES-192 encryption
    Aes192 = 0x0102,
    #! AES-256 encryption (recommended)
    Aes256 = 0x0103,
}

#! Allowed values for compression method fields in data provider types
public const CompressionMethodAllowedValues = (
    <AllowedValueInfo>{
        "value": CompressionMethod::Store,
        "display_name": "Store",
        "desc": "No compression (store only)",
    },
    <AllowedValueInfo>{
        "value": CompressionMethod::Deflate,
        "display_name": "Deflate",
        "desc": "Deflate compression (standard, most compatible)",
    },
    <AllowedValueInfo>{
        "value": CompressionMethod::BZip2,
        "display_name": "BZip2",
        "desc": "BZip2 compression",
    },
    <AllowedValueInfo>{
        "value": CompressionMethod::Lzma,
        "display_name": "LZMA",
        "desc": "LZMA compression",
    },
    <AllowedValueInfo>{
        "value": CompressionMethod::Zstd,
        "display_name": "Zstandard",
        "desc": "Zstandard compression (fast, high ratio)",
    },
    <AllowedValueInfo>{
        "value": CompressionMethod::Xz,
        "display_name": "XZ",
        "desc": "XZ/LZMA2 compression",
    },
);

#! Allowed values for compression level fields in data provider types
public const CompressionLevelAllowedValues = (
    <AllowedValueInfo>{
        "value": CompressionLevel::Default,
        "display_name": "Default",
        "desc": "Default compression level",
    },
    <AllowedValueInfo>{
        "value": CompressionLevel::Store,
        "display_name": "Store",
        "desc": "No compression",
    },
    <AllowedValueInfo>{
        "value": CompressionLevel::Fastest,
        "display_name": "Fastest",
        "desc": "Fastest compression",
    },
    <AllowedValueInfo>{
        "value": CompressionLevel::Best,
        "display_name": "Best",
        "desc": "Best (maximum) compression",
    },
);

#! Allowed values for encryption method fields in data provider types
public const EncryptionMethodAllowedValues = (
    <AllowedValueInfo>{
        "value": EncryptionMethod::None,
        "display_name": "None",
        "desc": "No encryption",
    },
    <AllowedValueInfo>{
        "value": EncryptionMethod::TradPkware,
        "display_name": "Traditional PKWARE",
        "desc": "Traditional PKWARE encryption (weak, not recommended)",
    },
    <AllowedValueInfo>{
        "value": EncryptionMethod::Aes128,
        "display_name": "AES-128",
        "desc": "AES-128 encryption",
    },
    <AllowedValueInfo>{
        "value": EncryptionMethod::Aes192,
        "display_name": "AES-192",
        "desc": "AES-192 encryption",
    },
    <AllowedValueInfo>{
        "value": EncryptionMethod::Aes256,
        "display_name": "AES-256",
        "desc": "AES-256 encryption (recommended)",
    },
);
}
