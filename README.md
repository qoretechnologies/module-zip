# Qore zip Module

## Introduction

The `zip` module provides comprehensive ZIP archive functionality for Qore, including:

- Creating, reading, and modifying ZIP archives
- Multiple compression methods: deflate, bzip2, lzma, zstd, xz
- ZIP64 support for large files (>4GB) and many entries (>65,535)
- AES encryption (128/192/256-bit)
- Streaming API for large file operations
- Data provider module for integration with Qore's data provider framework

## Requirements

- Qore 2.0+
- CMake 3.5+
- C++11 compiler
- zlib (required)
- bzip2 (optional, for bzip2 compression)
- liblzma (optional, for lzma/xz compression)
- zstd (optional, for zstandard compression)
- OpenSSL (optional, for AES encryption)

## Building

```bash
mkdir build
cd build
cmake ..
make
make install
```

## Quick Start

```qore
#!/usr/bin/qore

%requires zip

# Create a new archive
{
    ZipFile zip("archive.zip", "w");
    zip.addText("readme.txt", "Hello, World!");
    zip.addFile("document.pdf", "/path/to/document.pdf");
    zip.addDirectory("images/");
    zip.close();
}

# Read archive contents
{
    ZipFile zip("archive.zip", "r");
    foreach hash<ZipEntryInfo> entry in (zip.entries()) {
        printf("Entry: %s, Size: %d bytes\n", entry.name, entry.size);
    }
    zip.close();
}

# Extract archive
{
    ZipFile zip("archive.zip", "r");
    zip.extractAll("/destination/path");
    zip.close();
}
```

## Data Provider

The module includes `ZipDataProvider` for use with Qore's data provider framework:

```qore
%requires ZipDataProvider

# Use data provider actions
auto result = UserApi::callRestApi("zip", "create-archive", {
    "path": "output.zip",
    "files": (
        {"name": "file.txt", "data": "Hello, World!"},
    ),
});
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Copyright

Copyright 2026 Qore Technologies, s.r.o.
