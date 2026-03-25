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

    #! Output location URL (alternative to output_path)
    *string output_location;

    #! Entries to add: list of hashes with "name", "data" or "path", and optional "compression_method"
    list<hash<auto>> entries;

    #! Archive comment
    *string comment;

    #! Default compression method (0=store, 8=deflate)
    *int compression_method;

    #! Password for encryption (optional)
    *string password;

    #! Encryption method (one of the ZIP_EM_* constants; default: AES-256 when password set)
    *int encryption_method;
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

    #! Location URL for the archive (alternative to input_path or data)
    *string location;

    #! Destination directory
    string destination;

    #! Password for decryption (optional)
    *string password;

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Glob patterns for including entries (whitelist); if set, only matching entries are extracted
    *list<string> include_patterns;

    #! Glob patterns for excluding entries (blacklist); matching entries are skipped
    *list<string> exclude_patterns;

    #! Regex pattern for including entries (whitelist); if set, only matching entries are extracted
    *string include_regex;

    #! Regex pattern for excluding entries (blacklist); matching entries are skipped
    *string exclude_regex;

    #! If True, allow symlink entries to be extracted (default: False)
    *bool allow_symlinks;

    #! If True, overwrite existing files during extraction (default: False)
    *bool overwrite;

    #! If True, preserve directory structure (default: True)
    *bool preserve_paths;

    #! Strip this prefix from entry paths during extraction
    *string strip_prefix;

    #! Add this prefix to entry paths during extraction
    *string add_prefix;
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

    #! Number of entries skipped by filters
    *int skipped_count;

    #! List of skipped entry names (when filters are active)
    *list<string> skipped_files;
}

#! Request type for listing archive contents
public hashdecl ZipListArchiveRequest {
    #! Input file path
    *string input_path;

    #! Binary data of the archive (alternative to input_path)
    *binary data;

    #! Location URL for the archive (alternative to input_path or data)
    *string location;

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Glob patterns for including entries (whitelist)
    *list<string> include_patterns;

    #! Glob patterns for excluding entries (blacklist)
    *list<string> exclude_patterns;

    #! Regex pattern for including entries (whitelist)
    *string include_regex;

    #! Regex pattern for excluding entries (blacklist)
    *string exclude_regex;
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

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Location URL for the archive (alternative to input_path or data)
    *string location;

    #! Glob patterns for including entries (whitelist)
    *list<string> include_patterns;

    #! Glob patterns for excluding entries (blacklist)
    *list<string> exclude_patterns;

    #! Regex pattern for including entries (whitelist)
    *string include_regex;

    #! Regex pattern for excluding entries (blacklist)
    *string exclude_regex;
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

    #! Encryption method (one of the ZIP_EM_* constants; default: AES-256 when password set)
    *int encryption_method;
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

    #! Location URL for the archive (alternative to archive_path or archive_data)
    *string archive_location;

    #! Name of the entry to extract
    string entry_name;

    #! Password for decryption (optional)
    *string password;

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Output file path (optional, if not specified returns data)
    *string output_path;

    #! Output location URL (alternative to output_path)
    *string output_location;
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

    #! Output location URL (alternative to returning data in response)
    *string output_location;

    #! Encryption method (one of the ZIP_EM_* constants; default: AES-256 when password set)
    *int encryption_method;
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

#! Request type for adding a filesystem path to an archive
public hashdecl ZipAddPathRequest {
    #! Archive file path
    string archive_path;

    #! Filesystem path to add (file or directory)
    string source_path;

    #! Base path for computing relative entry names
    *string root_path;

    #! Password for encryption (optional)
    *string password;

    #! Compression method (0=store, 8=deflate)
    *int compression_method;

    #! Encryption method (one of the ZIP_EM_* constants; default: AES-256 when password set)
    *int encryption_method;
}

#! Response type for adding a filesystem path
public hashdecl ZipAddPathResponse {
    #! True if successful
    bool success;
}

#! Data type for add path requests
public class ZipAddPathRequestDataType inherits HashDataType {
    public {
        #! Field descriptions
        const Fields = {
            "archive_path": {
                "type": StringType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to add files to.",
                "example_value": "/path/to/archive.zip",
            },
            "source_path": {
                "type": StringType,
                "display_name": "Source Path",
                "short_desc": "File or directory to add",
                "desc": "Filesystem path to a file or directory to add recursively to the archive.",
                "example_value": "/path/to/directory",
            },
            "root_path": {
                "type": StringOrNothingType,
                "display_name": "Root Path",
                "short_desc": "Base path for relative entry names",
                "desc": "Base path for computing relative entry names in the archive. "
                    "For example, if source_path='/a/b/c' and root_path='/a', entries will be "
                    "stored as 'b/c/...'.",
                "example_value": "/path/to",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Encryption password",
                "desc": "Password for AES-256 encryption of new entries.",
                "sensitive": True,
            },
            "compression_method": {
                "type": IntType,
                "display_name": "Compression Method",
                "short_desc": "Compression algorithm",
                "desc": "Compression method for all entries.",
                "default_value": CompressionMethod::Deflate,
                "allowed_values": CompressionMethodAllowedValues,
            },
            "encryption_method": {
                "type": IntOrNothingType,
                "display_name": "Encryption Method",
                "short_desc": "Encryption algorithm",
                "desc": "Encryption method to use when a password is provided. Defaults to AES-256.",
                "default_value": EncryptionMethod::Aes256,
                "allowed_values": EncryptionMethodAllowedValues,
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for add path responses
public class ZipAddPathResponseDataType inherits HashDataType {
    private {
        #! Field descriptions
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "`True` if the path was added successfully.",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
    }
}

#! Request type for decompressing data
public hashdecl ZipDecompressDataRequest {
    #! Compressed archive data
    *binary data;

    #! Archive file path (alternative to data)
    *string archive_path;

    #! Location URL for the archive (alternative to data or archive_path)
    *string archive_location;

    #! Password for decryption (optional)
    *string password;

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Specific entry names to extract (optional, extracts all if not specified)
    *list<string> entry_names;

    #! Glob patterns for including entries (whitelist)
    *list<string> include_patterns;

    #! Glob patterns for excluding entries (blacklist)
    *list<string> exclude_patterns;

    #! Regex pattern for including entries (whitelist)
    *string include_regex;

    #! Regex pattern for excluding entries (blacklist)
    *string exclude_regex;
}

#! Response type for decompressing data
public hashdecl ZipDecompressDataResponse {
    #! Hash of entry name -> binary data
    hash<string, binary> entries;

    #! Number of entries extracted
    int entry_count;
}

#! Request type for verifying archive integrity
public hashdecl ZipVerifyArchiveRequest {
    #! Input file path
    *string input_path;

    #! Binary data of the archive (alternative to input_path)
    *binary data;

    #! Location URL for the archive (alternative to input_path or data)
    *string location;

    #! Password for decryption (optional)
    *string password;

    #! If True, enable recovery mode for corrupted archives
    *bool recover;

    #! Glob patterns for including entries (whitelist)
    *list<string> include_patterns;

    #! Glob patterns for excluding entries (blacklist)
    *list<string> exclude_patterns;

    #! Regex pattern for including entries (whitelist)
    *string include_regex;

    #! Regex pattern for excluding entries (blacklist)
    *string exclude_regex;
}

#! Response type for verifying archive integrity
public hashdecl ZipVerifyArchiveResponse {
    #! True if all verified entries are valid
    bool valid;

    #! Number of entries verified
    int verified_count;

    #! Number of entries that failed verification
    int failed_count;

    #! Per-entry verification results
    list<hash<auto>> results;
}

#! Request type for replacing or deleting entries
public hashdecl ZipReplaceEntriesRequest {
    #! Archive file path
    string archive_path;

    #! Hash mapping entry names to replacement data (binary or hash with "data" key)
    *hash<auto> replacements;

    #! List of entry names to delete
    *list<string> delete_names;

    #! Password for encrypted entries (optional)
    *string password;
}

#! Response type for replacing or deleting entries
public hashdecl ZipReplaceEntriesResponse {
    #! True if successful
    bool success;

    #! Number of entries replaced
    int replaced_count;

    #! Number of entries deleted
    int deleted_count;

    #! Number of entries copied unchanged
    int copied_count;
}

#! Data type for create archive requests
/** Provides rich metadata for the create archive request including field descriptions,
    display names, and allowed values for compression methods.
*/
public class ZipCreateArchiveRequestDataType inherits HashDataType {
    public {
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
            "output_location": {
                "type": StringOrNothingType,
                "display_name": "Output Location",
                "short_desc": "Output location URL",
                "desc": "A FileLocationHandler location URL where the archive will be written "
                    "(alternative to output_path).",
                "example_value": "file:///path/to/archive.zip",
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
                "type": IntType,
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
            "encryption_method": {
                "type": IntOrNothingType,
                "display_name": "Encryption Method",
                "short_desc": "Encryption algorithm",
                "desc": "Encryption method to use when a password is provided. Defaults to AES-256.",
                "default_value": EncryptionMethod::Aes256,
                "allowed_values": EncryptionMethodAllowedValues,
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
                "desc": "`True` if the archive was created successfully.",
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
    public {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to extract.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive.",
            },
            "location": {
                "type": StringOrNothingType,
                "display_name": "Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive (alternative to input_path "
                    "or data).",
                "example_value": "file:///path/to/archive.zip",
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

        #! Extract option fields
        const ExtractOptionFields = {
            "allow_symlinks": {
                "type": SoftBoolOrNothingType,
                "display_name": "Allow Symlinks",
                "short_desc": "Allow symlink extraction",
                "desc": "If `True`, allow symlink entries to be extracted. Symlinks are a security "
                    "risk as they can point outside the destination directory. Default is `False` "
                    "(symlinks are silently skipped). When enabled, symlink targets are still "
                    "validated to stay within the destination directory.",
                "default_value": False,
            },
            "overwrite": {
                "type": SoftBoolOrNothingType,
                "display_name": "Overwrite",
                "short_desc": "Overwrite existing files",
                "desc": "If `True`, overwrite existing files during extraction. Default is `False` "
                    "(skip existing files).",
                "default_value": False,
            },
            "preserve_paths": {
                "type": SoftBoolOrNothingType,
                "display_name": "Preserve Paths",
                "short_desc": "Preserve directory structure",
                "desc": "If `True` (default), preserve directory paths during extraction. If `False`, "
                    "all entries are extracted as flat files in the destination directory.",
                "default_value": True,
            },
            "strip_prefix": {
                "type": StringOrNothingType,
                "display_name": "Strip Prefix",
                "short_desc": "Strip prefix from entry paths",
                "desc": "If set, entries whose names start with this prefix will have it removed. "
                    "For example, with `strip_prefix` = `src/`, the entry `src/main.cpp` becomes `main.cpp`.",
                "example_value": "src/",
            },
            "add_prefix": {
                "type": StringOrNothingType,
                "display_name": "Add Prefix",
                "short_desc": "Add prefix to entry paths",
                "desc": "If set, this prefix is prepended to all entry names during extraction. "
                    "For example, with `add_prefix` = `output/`, the entry `main.cpp` becomes `output/main.cpp`.",
                "example_value": "output/",
            },
        };

        #! Filter fields
        const FilterFields = {
            "include_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Include Patterns",
                "short_desc": "Glob whitelist patterns",
                "desc": "List of glob patterns to include (whitelist). Only entries matching at least "
                    "one pattern will be included. Use `*` to match any characters and `?` to match "
                    "a single character.",
                "example_value": ("*.txt", "data/*"),
            },
            "exclude_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Exclude Patterns",
                "short_desc": "Glob blacklist patterns",
                "desc": "List of glob patterns to exclude (blacklist). Entries matching any pattern "
                    "will be skipped. Exclude is applied after include (blacklist overrides whitelist).",
                "example_value": ("*.log", "temp_*"),
            },
            "include_regex": {
                "type": StringOrNothingType,
                "display_name": "Include Regex",
                "short_desc": "Regex whitelist pattern",
                "desc": "Regular expression pattern for including entries (whitelist). Only entries "
                    "whose names match the regex will be included.",
                "example_value": "^src/.*\\.cpp$",
            },
            "exclude_regex": {
                "type": StringOrNothingType,
                "display_name": "Exclude Regex",
                "short_desc": "Regex blacklist pattern",
                "desc": "Regular expression pattern for excluding entries (blacklist). Entries whose "
                    "names match the regex will be skipped.",
                "example_value": "\\.tmp$",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        addQoreFields(ExtractOptionFields);
        addQoreFields(FilterFields);
        setRequiredGroups((("input_path", "data", "location"),));
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
                "desc": "`True` if the archive was extracted successfully.",
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
            "skipped_count": {
                "type": IntOrNothingType,
                "display_name": "Skipped Count",
                "short_desc": "Number of entries skipped by filters",
                "desc": "The number of entries that were skipped due to filter settings. "
                    "Only present when filters are active.",
            },
            "skipped_files": {
                "type": SoftListOrNothingType,
                "display_name": "Skipped Files",
                "short_desc": "List of skipped entry names",
                "desc": "List of entry names that were skipped due to filter settings. "
                    "Only present when filters are active.",
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
    public {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to list.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive.",
            },
            "location": {
                "type": StringOrNothingType,
                "display_name": "Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive (alternative to input_path "
                    "or data).",
                "example_value": "file:///path/to/archive.zip",
            },
            "include_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Include Patterns",
                "short_desc": "Glob whitelist patterns",
                "desc": "List of glob patterns to include (whitelist). Only entries matching at least "
                    "one pattern will be included. Use `*` to match any characters and `?` to match "
                    "a single character.",
                "example_value": ("*.txt", "data/*"),
            },
            "exclude_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Exclude Patterns",
                "short_desc": "Glob blacklist patterns",
                "desc": "List of glob patterns to exclude (blacklist). Entries matching any pattern "
                    "will be skipped. Exclude is applied after include (blacklist overrides whitelist).",
                "example_value": ("*.log", "temp_*"),
            },
            "include_regex": {
                "type": StringOrNothingType,
                "display_name": "Include Regex",
                "short_desc": "Regex whitelist pattern",
                "desc": "Regular expression pattern for including entries (whitelist). Only entries "
                    "whose names match the regex will be included.",
                "example_value": "^src/.*\\.cpp$",
            },
            "exclude_regex": {
                "type": StringOrNothingType,
                "display_name": "Exclude Regex",
                "short_desc": "Regex blacklist pattern",
                "desc": "Regular expression pattern for excluding entries (blacklist). Entries whose "
                    "names match the regex will be skipped.",
                "example_value": "\\.tmp$",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        setRequiredGroups((("input_path", "data", "location"),));
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
    public {
        #! Field descriptions
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to get information about.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive.",
            },
            "location": {
                "type": StringOrNothingType,
                "display_name": "Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive (alternative to input_path "
                    "or data).",
                "example_value": "file:///path/to/archive.zip",
            },
            "include_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Include Patterns",
                "short_desc": "Glob whitelist patterns",
                "desc": "List of glob patterns to include (whitelist). Only entries matching at least "
                    "one pattern will be included. Use `*` to match any characters and `?` to match "
                    "a single character.",
                "example_value": ("*.txt", "data/*"),
            },
            "exclude_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Exclude Patterns",
                "short_desc": "Glob blacklist patterns",
                "desc": "List of glob patterns to exclude (blacklist). Entries matching any pattern "
                    "will be skipped. Exclude is applied after include (blacklist overrides whitelist).",
                "example_value": ("*.log", "temp_*"),
            },
            "include_regex": {
                "type": StringOrNothingType,
                "display_name": "Include Regex",
                "short_desc": "Regex whitelist pattern",
                "desc": "Regular expression pattern for including entries (whitelist). Only entries "
                    "whose names match the regex will be included.",
                "example_value": "^src/.*\\.cpp$",
            },
            "exclude_regex": {
                "type": StringOrNothingType,
                "display_name": "Exclude Regex",
                "short_desc": "Regex blacklist pattern",
                "desc": "Regular expression pattern for excluding entries (blacklist). Entries whose "
                    "names match the regex will be skipped.",
                "example_value": "\\.tmp$",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        setRequiredGroups((("input_path", "data", "location"),));
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
    public {
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
            "encryption_method": {
                "type": IntOrNothingType,
                "display_name": "Encryption Method",
                "short_desc": "Encryption algorithm",
                "desc": "Encryption method to use when a password is provided. Defaults to AES-256.",
                "default_value": EncryptionMethod::Aes256,
                "allowed_values": EncryptionMethodAllowedValues,
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
                "desc": "`True` if the files were added successfully.",
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
    public {
        #! Field descriptions
        const Fields = {
            "archive_path": {
                "type": StringOrNothingType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive.",
                "example_value": "/path/to/archive.zip",
            },
            "archive_data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive.",
            },
            "archive_location": {
                "type": StringOrNothingType,
                "display_name": "Archive Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive (alternative to archive_path "
                    "or archive_data).",
                "example_value": "file:///path/to/archive.zip",
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
            "output_location": {
                "type": StringOrNothingType,
                "display_name": "Output Location",
                "short_desc": "Output location URL",
                "desc": "A FileLocationHandler location URL where the extracted file should be written "
                    "(alternative to output_path).",
                "example_value": "file:///path/to/output/file.txt",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        setRequiredGroups((("archive_path", "archive_data", "archive_location"),));
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
                "desc": "`True` if the file was extracted successfully.",
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
    public {
        #! Field descriptions
        const Fields = {
            "entries": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Entries",
                "short_desc": "List of entries to compress",
                "desc": "List of entries to compress. Each entry should have 'name' and 'data' (binary data).",
            },
            "compression_method": {
                "type": IntType,
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
            "encryption_method": {
                "type": IntOrNothingType,
                "display_name": "Encryption Method",
                "short_desc": "Encryption algorithm",
                "desc": "Encryption method to use when a password is provided. Defaults to AES-256.",
                "default_value": EncryptionMethod::Aes256,
                "allowed_values": EncryptionMethodAllowedValues,
            },
            "output_location": {
                "type": StringOrNothingType,
                "display_name": "Output Location",
                "short_desc": "Output location URL",
                "desc": "A FileLocationHandler location URL where the compressed archive will be written. "
                    "If not specified, the archive data is returned in the response.",
                "example_value": "file:///path/to/archive.zip",
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
    public {
        #! Field descriptions
        const Fields = {
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Compressed Data",
                "short_desc": "Compressed archive data",
                "desc": "Binary data containing the ZIP archive to decompress.",
            },
            "archive_path": {
                "type": StringOrNothingType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to decompress (alternative to data).",
                "example_value": "/path/to/archive.zip",
            },
            "archive_location": {
                "type": StringOrNothingType,
                "display_name": "Archive Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive (alternative to data "
                    "or archive_path).",
                "example_value": "file:///path/to/archive.zip",
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
            "include_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Include Patterns",
                "short_desc": "Glob whitelist patterns",
                "desc": "List of glob patterns to include (whitelist). Only entries matching at least "
                    "one pattern will be included. Use `*` to match any characters and `?` to match "
                    "a single character.",
                "example_value": ("*.txt", "data/*"),
            },
            "exclude_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Exclude Patterns",
                "short_desc": "Glob blacklist patterns",
                "desc": "List of glob patterns to exclude (blacklist). Entries matching any pattern "
                    "will be skipped. Exclude is applied after include (blacklist overrides whitelist).",
                "example_value": ("*.log", "temp_*"),
            },
            "include_regex": {
                "type": StringOrNothingType,
                "display_name": "Include Regex",
                "short_desc": "Regex whitelist pattern",
                "desc": "Regular expression pattern for including entries (whitelist). Only entries "
                    "whose names match the regex will be included.",
                "example_value": "^src/.*\\.cpp$",
            },
            "exclude_regex": {
                "type": StringOrNothingType,
                "display_name": "Exclude Regex",
                "short_desc": "Regex blacklist pattern",
                "desc": "Regular expression pattern for excluding entries (blacklist). Entries whose "
                    "names match the regex will be skipped.",
                "example_value": "\\.tmp$",
            },
        };
    }

    #! Creates the object
    constructor() {
        addQoreFields(Fields);
        setRequiredGroups((("data", "archive_path", "archive_location"),));
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

#! Data type for verify archive requests
public class ZipVerifyArchiveRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "input_path": {
                "type": StringOrNothingType,
                "display_name": "Input Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to verify.",
                "example_value": "/path/to/archive.zip",
            },
            "data": {
                "type": BinaryOrNothingType,
                "display_name": "Archive Data",
                "short_desc": "Binary archive data",
                "desc": "Binary data containing the ZIP archive.",
            },
            "location": {
                "type": StringOrNothingType,
                "display_name": "Location",
                "short_desc": "Archive location URL",
                "desc": "A FileLocationHandler location URL for the archive.",
                "example_value": "file:///path/to/archive.zip",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Decryption password",
                "desc": "Password for decrypting encrypted entries.",
                "sensitive": True,
            },
            "recover": {
                "type": SoftBoolOrNothingType,
                "display_name": "Recovery Mode",
                "short_desc": "Enable recovery for corrupted archives",
                "desc": "If `True`, enable recovery mode to read archives with corrupted "
                    "central directories by reading local file headers.",
                "default_value": False,
            },
            "include_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Include Patterns",
                "short_desc": "Glob whitelist patterns",
                "desc": "List of glob patterns to include (whitelist).",
                "example_value": ("*.txt", "data/*"),
            },
            "exclude_patterns": {
                "type": SoftListOrNothingType,
                "display_name": "Exclude Patterns",
                "short_desc": "Glob blacklist patterns",
                "desc": "List of glob patterns to exclude (blacklist).",
                "example_value": ("*.log", "temp_*"),
            },
            "include_regex": {
                "type": StringOrNothingType,
                "display_name": "Include Regex",
                "short_desc": "Regex whitelist pattern",
                "desc": "Regular expression pattern for including entries.",
            },
            "exclude_regex": {
                "type": StringOrNothingType,
                "display_name": "Exclude Regex",
                "short_desc": "Regex blacklist pattern",
                "desc": "Regular expression pattern for excluding entries.",
            },
        };
    }

    constructor() {
        addQoreFields(Fields);
        setRequiredGroups((("input_path", "data", "location"),));
    }
}

#! Data type for verify archive responses
public class ZipVerifyArchiveResponseDataType inherits HashDataType {
    private {
        const Fields = {
            "valid": {
                "type": BoolType,
                "display_name": "Valid",
                "short_desc": "Overall verification result",
                "desc": "`True` if all verified entries passed integrity checks.",
            },
            "verified_count": {
                "type": IntType,
                "display_name": "Verified Count",
                "short_desc": "Number of entries verified",
                "desc": "The total number of entries that were verified.",
            },
            "failed_count": {
                "type": IntType,
                "display_name": "Failed Count",
                "short_desc": "Number of entries that failed",
                "desc": "The number of entries that failed integrity verification.",
            },
            "results": {
                "type": new ListDataType(new HashDataType(), True),
                "display_name": "Results",
                "short_desc": "Per-entry verification results",
                "desc": "List of per-entry results with `name`, `valid`, and optional `error` fields.",
            },
        };
    }

    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for replace entries requests
public class ZipReplaceEntriesRequestDataType inherits HashDataType {
    public {
        const Fields = {
            "archive_path": {
                "type": StringType,
                "display_name": "Archive Path",
                "short_desc": "Path to the archive file",
                "desc": "Filesystem path to the ZIP archive to modify.",
                "example_value": "/path/to/archive.zip",
            },
            "replacements": {
                "type": new HashDataType(),
                "display_name": "Replacements",
                "short_desc": "Entries to replace",
                "desc": "Hash mapping entry names to replacement data. Values can be binary data "
                    "or a hash with a `data` key containing binary data.",
            },
            "delete_names": {
                "type": SoftListOrNothingType,
                "display_name": "Delete Entries",
                "short_desc": "Entry names to delete",
                "desc": "List of entry names to remove from the archive.",
            },
            "password": {
                "type": StringOrNothingType,
                "display_name": "Password",
                "short_desc": "Encryption password",
                "desc": "Password for encrypted entries.",
                "sensitive": True,
            },
        };
    }

    constructor() {
        addQoreFields(Fields);
    }
}

#! Data type for replace entries responses
public class ZipReplaceEntriesResponseDataType inherits HashDataType {
    private {
        const Fields = {
            "success": {
                "type": BoolType,
                "display_name": "Success",
                "short_desc": "Operation success status",
                "desc": "`True` if the operation completed successfully.",
            },
            "replaced_count": {
                "type": IntType,
                "display_name": "Replaced Count",
                "short_desc": "Number of entries replaced",
                "desc": "The number of entries that were replaced with new data.",
            },
            "deleted_count": {
                "type": IntType,
                "display_name": "Deleted Count",
                "short_desc": "Number of entries deleted",
                "desc": "The number of entries that were removed from the archive.",
            },
            "copied_count": {
                "type": IntType,
                "display_name": "Copied Count",
                "short_desc": "Number of entries copied unchanged",
                "desc": "The number of entries that were copied to the new archive unchanged.",
            },
        };
    }

    constructor() {
        addQoreFields(Fields);
    }
}
}
