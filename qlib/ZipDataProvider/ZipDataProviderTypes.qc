# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipDataProvider type definitions

/** ZipDataProviderTypes.qc Copyright (C) 2026 Qore Technologies, s.r.o.

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
#! Request type for creating a ZIP archive
public hashdecl ZipCreateArchiveRequest {
    #! Output file path (optional if returning binary data)
    *string output_path;

    #! Entries to add: list of hashes with "name", "data" or "path", and optional "compression_method"
    list<hash<auto>> entries;

    #! Archive comment
    *string comment;

    #! Default compression method (0=store, 8=deflate)
    *int compression_method;

    #! Password for encryption (optional)
    *string password;
}

#! Response type for creating a ZIP archive
public hashdecl ZipCreateArchiveResponse {
    #! True if successful
    bool success;

    #! Output file path if written to disk
    *string output_path;

    #! Binary data if in-memory archive
    *binary data;

    #! Number of entries added
    int entry_count;

    #! Archive size in bytes
    int size;
}

#! Request type for extracting a ZIP archive
public hashdecl ZipExtractArchiveRequest {
    #! Input file path
    *string input_path;

    #! Binary data of the archive (alternative to input_path)
    *binary data;

    #! Destination directory
    string destination;

    #! Password for decryption (optional)
    *string password;
}

#! Response type for extracting a ZIP archive
public hashdecl ZipExtractArchiveResponse {
    #! True if successful
    bool success;

    #! Number of entries extracted
    int entry_count;

    #! Destination directory
    string destination;

    #! List of extracted file paths
    list<string> extracted_files;
}

#! Request type for listing archive contents
public hashdecl ZipListArchiveRequest {
    #! Input file path
    *string input_path;

    #! Binary data of the archive (alternative to input_path)
    *binary data;
}

#! Response type for listing archive contents
public hashdecl ZipListArchiveResponse {
    #! List of entries with their metadata
    list<hash<ZipEntryInfo>> entries;

    #! Total number of entries
    int count;
}

#! Request type for getting archive info
public hashdecl ZipArchiveInfoRequest {
    #! Input file path
    *string input_path;

    #! Binary data of the archive (alternative to input_path)
    *binary data;
}

#! Response type for getting archive info
public hashdecl ZipArchiveInfoResponse {
    #! Archive file path (if from file)
    *string path;

    #! Total number of entries
    int entry_count;

    #! Total uncompressed size
    int total_size;

    #! Total compressed size
    int compressed_size;

    #! Archive comment
    *string comment;
}

#! Request type for adding files to archive
public hashdecl ZipAddFilesRequest {
    #! Archive file path
    string archive_path;

    #! Entries to add: list of hashes with "name", "data" or "path"
    list<hash<auto>> entries;

    #! Password for new entries (optional)
    *string password;
}

#! Response type for adding files to archive
public hashdecl ZipAddFilesResponse {
    #! True if successful
    bool success;

    #! Number of entries added
    int entries_added;
}

#! Request type for extracting a single file
public hashdecl ZipExtractFileRequest {
    #! Archive file path
    *string archive_path;

    #! Binary data of the archive (alternative to archive_path)
    *binary archive_data;

    #! Name of the entry to extract
    string entry_name;

    #! Password for decryption (optional)
    *string password;

    #! Output file path (optional, if not specified returns data)
    *string output_path;
}

#! Response type for extracting a single file
public hashdecl ZipExtractFileResponse {
    #! True if successful
    bool success;

    #! Entry name
    string entry_name;

    #! Binary data (if not written to file)
    *binary data;

    #! Output file path (if written to file)
    *string output_path;

    #! Size of extracted data
    int size;
}

#! Request type for compressing data
public hashdecl ZipCompressDataRequest {
    #! Entries to compress: list of hashes with "name" and "data"
    list<hash<auto>> entries;

    #! Compression method (0=store, 8=deflate)
    *int compression_method;

    #! Password for encryption (optional)
    *string password;
}

#! Response type for compressing data
public hashdecl ZipCompressDataResponse {
    #! Compressed archive data
    binary data;

    #! Number of entries
    int entry_count;

    #! Compressed size
    int size;
}

#! Request type for decompressing data
public hashdecl ZipDecompressDataRequest {
    #! Compressed archive data
    binary data;

    #! Password for decryption (optional)
    *string password;

    #! Specific entry names to extract (optional, extracts all if not specified)
    *list<string> entry_names;
}

#! Response type for decompressing data
public hashdecl ZipDecompressDataResponse {
    #! Hash of entry name -> binary data
    hash<string, binary> entries;

    #! Number of entries extracted
    int entry_count;
}

#! Data type for create archive requests
/** Provides rich metadata for the create archive request including field descriptions,
    display names, and allowed values for compression methods.
*/
public class ZipCreateArchiveRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "output_path": {
                "type": StringOrNothingType,
                "display_name": "Output Path",
                "short_desc": "Output file path for the archive",
                "desc": "Output file path where the archive will be written. If not specified, the "
                    "archive data is returned as binary data in the response.",
                "example_value": "/path/to/archive.zip",
            },
            "entries": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Entries",
                "short_desc": "List of entries to add to the archive",
                "desc": "List of entries to add. Each entry should have 'name' and either 'data' "
                    "(binary data) or 'path' (filesystem path to read from). Optional 'compression_method' "
                    "per entry overrides the default.",
            },
            "comment": {
                "type": StringOrNothingType,
                "display_name": "Archive Comment",
                "short_desc": "Optional archive comment",
                "desc": "Optional comment to embed in the archive metadata.",
                "example_value": "Created by Qore ZipDataProvider",
            },
            "compression_method": {
                "type": IntOrNothingType,
                "display_name": "Compression Method",
                "short_desc": "Default compression algorithm",
                "desc": "Default compression method for all entries. Can be overridden per entry.",
                "default_value": CompressionMethod::Deflate,
                "allowed_values": CompressionMethodAllowedValues,
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Encryption password",
                "desc": "Password for AES-256 encryption. If provided, all entries will be encrypted.",
                "sensitive": True,
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for create archive responses
public class ZipCreateArchiveResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "True if the archive was created successfully.",
            },
            "output_path": {
                "type": StringOrNothingType,
                "display_name": "Output Path",
                "short_desc": "Path where the archive was written",
                "desc": "The filesystem path where the archive was written, if output_path was specified.",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "The archive as binary data if no output_path was specified.",
            },
            "entry_count": {
                "type": IntType,
                "display_name": "Entry Count",
                "short_desc": "Number of entries added",
                "desc": "The total number of entries added to the archive.",
            },
            "size": {
                "type": IntType,
                "display_name": "Archive Size",
                "short_desc": "Archive size in bytes",
                "desc": "The size of the archive in bytes.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for extract archive requests
public class ZipExtractArchiveRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to extract. Either input_path or data must "
                    "be provided.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive. Either input_path or data must be provided.",
            },
            "destination": {
                "type": StringType,
                "display_name": "Destination Directory",
                "short_desc": "Directory to extract files to",
                "desc": "Filesystem path to the destination directory where files will be extracted.",
                "example_value": "/path/to/destination",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Decryption password",
                "desc": "Password for decrypting encrypted entries.",
                "sensitive": True,
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        # Require either input_path or data
        setRequiredGroups((("input_path", "data"),));
    }
}

#! Data type for extract archive responses
public class ZipExtractArchiveResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "True if the archive was extracted successfully.",
            },
            "entry_count": {
                "type": IntType,
                "display_name": "Entry Count",
                "short_desc": "Number of entries extracted",
                "desc": "The total number of entries extracted from the archive.",
            },
            "destination": {
                "type": StringType,
                "display_name": "Destination",
                "short_desc": "Extraction destination directory",
                "desc": "The filesystem path where files were extracted.",
            },
            "extracted_files": {
                "type": new ListDataType(StringType, True),
                "display_name": "Extracted Files",
                "short_desc": "List of extracted file paths",
                "desc": "List of filesystem paths to all extracted files.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for list archive requests
public class ZipListArchiveRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to list. Either input_path or data must "
                    "be provided.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive. Either input_path or data must be provided.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        # Require either input_path or data
        setRequiredGroups((("input_path", "data"),));
    }
}

#! Data type for list archive responses
public class ZipListArchiveResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "entries": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Entries",
                "short_desc": "List of archive entries",
                "desc": "List of entry information hashes containing name, size, compressed_size, "
                    "modified, crc32, compression_method, is_directory, is_encrypted, and comment.",
            },
            "count": {
                "type": IntType,
                "display_name": "Count",
                "short_desc": "Total number of entries",
                "desc": "The total number of entries in the archive.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for archive info requests
public class ZipArchiveInfoRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to get information about. Either input_path "
                    "or data must be provided.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive. Either input_path or data must be provided.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        # Require either input_path or data
        setRequiredGroups((("input_path", "data"),));
    }
}

#! Data type for archive info responses
public class ZipArchiveInfoResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "path": {
                "type": StringOrNothingType,
                "display_name": "Path",
                "short_desc": "Archive file path",
                "desc": "The filesystem path of the archive if opened from a file.",
            },
            "entry_count": {
                "type": IntType,
                "display_name": "Entry Count",
                "short_desc": "Number of entries in the archive",
                "desc": "The total number of entries in the archive.",
            },
            "total_size": {
                "type": IntType,
                "display_name": "Total Size",
                "short_desc": "Total uncompressed size",
                "desc": "The total uncompressed size of all entries in bytes.",
            },
            "compressed_size": {
                "type": IntType,
                "display_name": "Compressed Size",
                "short_desc": "Total compressed size",
                "desc": "The total compressed size of all entries in bytes.",
            },
            "comment": {
                "type": StringOrNothingType,
                "display_name": "Comment",
                "short_desc": "Archive comment",
                "desc": "The archive comment if one is set.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for add files requests
public class ZipAddFilesRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "archive_path": {
                "type": StringType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to add files to.",
                "example_value": "/path/to/archive.zip",
            },
            "entries": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Entries",
                "short_desc": "List of entries to add",
                "desc": "List of entries to add. Each entry should have 'name' and either 'data' "
                    "(binary data) or 'path' (filesystem path to read from).",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Encryption password for new entries",
                "desc": "Password for AES-256 encryption of new entries.",
                "sensitive": True,
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for add files responses
public class ZipAddFilesResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "True if the files were added successfully.",
            },
            "entries_added": {
                "type": IntType,
                "display_name": "Entries Added",
                "short_desc": "Number of entries added",
                "desc": "The number of entries that were added to the archive.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for extract file requests
public class ZipExtractFileRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "archive_path": {
                "type": StringOrNothingType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive. Either archive_path or archive_data must "
                    "be provided.",
                "example_value": "/path/to/archive.zip",
            },
            "archive_data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive. Either archive_path or archive_data "
                    "must be provided.",
            },
            "entry_name": {
                "type": StringType,
                "display_name": "Entry Name",
                "short_desc": "Name of the entry to extract",
                "desc": "The name/path of the entry within the archive to extract.",
                "example_value": "document.txt",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Decryption password",
                "desc": "Password for decrypting the entry if it is encrypted.",
                "sensitive": True,
            },
            "output_path": {
                "type": StringOrNothingType,
                "display_name": "Output Path",
                "short_desc": "Path to write the extracted file",
                "desc": "Filesystem path where the extracted file should be written. If not specified, "
                    "the file data is returned in the response.",
                "example_value": "/path/to/output/file.txt",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        # Require either archive_path or archive_data
        setRequiredGroups((("archive_path", "archive_data"),));
    }
}

#! Data type for extract file responses
public class ZipExtractFileResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "True if the file was extracted successfully.",
            },
            "entry_name": {
                "type": StringType,
                "display_name": "Entry Name",
                "short_desc": "Name of the extracted entry",
                "desc": "The name/path of the entry that was extracted.",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "File Data",
                "short_desc": "Extracted file data",
                "desc": "The extracted file as binary data if no output_path was specified.",
            },
            "output_path": {
                "type": StringOrNothingType,
                "display_name": "Output Path",
                "short_desc": "Path where file was written",
                "desc": "The filesystem path where the file was written if output_path was specified.",
            },
            "size": {
                "type": IntType,
                "display_name": "Size",
                "short_desc": "Extracted file size",
                "desc": "The size of the extracted file in bytes.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for compress data requests
public class ZipCompressDataRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "entries": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Entries",
                "short_desc": "List of entries to compress",
                "desc": "List of entries to compress. Each entry should have 'name' and 'data' (binary data).",
            },
            "compression_method": {
                "type": IntOrNothingType,
                "display_name": "Compression Method",
                "short_desc": "Compression algorithm",
                "desc": "Compression method to use for all entries.",
                "default_value": CompressionMethod::Deflate,
                "allowed_values": CompressionMethodAllowedValues,
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Encryption password",
                "desc": "Password for AES-256 encryption. If provided, all entries will be encrypted.",
                "sensitive": True,
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for compress data responses
public class ZipCompressDataResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "data": {
                "type": BinaryType,
                "display_name": "Compressed Data",
                "short_desc": "Compressed archive data",
                "desc": "The compressed ZIP archive as binary data.",
            },
            "entry_count": {
                "type": IntType,
                "display_name": "Entry Count",
                "short_desc": "Number of entries compressed",
                "desc": "The number of entries in the compressed archive.",
            },
            "size": {
                "type": IntType,
                "display_name": "Size",
                "short_desc": "Compressed size in bytes",
                "desc": "The size of the compressed archive in bytes.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for decompress data requests
public class ZipDecompressDataRequestDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "data": {
                "type": BinaryType,
                "display_name": "Compressed Data",
                "short_desc": "Compressed archive data",
                "desc": "Binary data containing the ZIP archive to decompress.",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Decryption password",
                "desc": "Password for decrypting encrypted entries.",
                "sensitive": True,
            },
            "entry_names": {
                "type": SoftListOrNothingType,
                "display_name": "Entry Names",
                "short_desc": "Specific entries to extract",
                "desc": "Optional list of specific entry names to extract. If not specified, all "
                    "entries are extracted.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for decompress data responses
public class ZipDecompressDataResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "entries": {
                "type": new HashDataType(),
                "display_name": "Entries",
                "short_desc": "Extracted entry data",
                "desc": "Hash mapping entry names to their binary data.",
            },
            "entry_count": {
                "type": IntType,
                "display_name": "Entry Count",
                "short_desc": "Number of entries extracted",
                "desc": "The number of entries that were extracted.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}
}
