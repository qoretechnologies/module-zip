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
}
