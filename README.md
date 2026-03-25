# Qore zip Module

## Overview

The `zip` module provides comprehensive ZIP archive functionality for Qore, built on
[minizip-ng](https://github.com/zlib-ng/minizip-ng). It includes a C++ binary module (`zip`),
a data provider module (`ZipDataProvider`) with 13 actions, and a command-line tool (`qzip`).

## Features

- **Archive operations**: create, read, extract, verify, diff, replace/delete entries, recompress
- **Compression**: store, deflate, bzip2, lzma, zstd, xz with configurable levels
- **Encryption**: AES-128/192/256 and PKWARE traditional with selectable method
- **Filtering**: glob and regex whitelist/blacklist for selective extraction
- **Path remapping**: strip_prefix, add_prefix, preserve_paths, flat extraction
- **Symlink security**: symlinks blocked by default, validated extraction with `allow_symlinks`
- **Integrity verification**: CRC32 verification without extraction
- **Archive recovery**: read corrupted archives via local file header recovery
- **Streaming API**: `ZipInputStream` / `ZipOutputStream` for large files
- **Rich metadata**: Unix permissions, symlink targets, encryption method, creator system
- **ZIP64**: transparent support for archives >4GB and >65,535 entries
- **Data provider API**: 13 actions with full UI metadata for Qorus integration
- **CLI tool**: `qzip` with 11 commands and colorized output

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
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

## Quick Start

### Creating an Archive

```qore
%requires zip

ZipFile zip("archive.zip", "w");
zip.addText("readme.txt", "Hello, World!");
zip.addFile("document.pdf", "/path/to/document.pdf");
zip.addDirectory("images/");
zip.addPath("/path/to/src", "/path/to");  # recursive directory add
zip.close();
```

### Reading and Listing

```qore
ZipFile zip("archive.zip", "r");
foreach hash<ZipEntryInfo> entry in (zip.entries()) {
    printf("%-40s %8d  %s  %s\n", entry.name, entry.size,
        entry.is_encrypted ? "ENC" : "   ",
        entry.is_symlink ? "-> " + entry.link_target : "");
}
zip.close();
```

### Extracting with Filters

```qore
ZipFile zip("archive.zip", "r");

# Extract only .cpp files, skip temp files
list<string> names = map $1.name, zip.entries(),
    $1.name =~ /\.cpp$/ && $1.name !~ /^temp/;

zip.extractEntries("/dest", names, {
    "overwrite": True,
    "strip_prefix": "src/",
    "entry_callback": sub (string name) { printf("  x %s\n", name); },
});
zip.close();
```

### Encryption

```qore
# AES-256 (default)
zip.addText("secret.txt", "data", NOTHING, {"password": "secret"});

# PKWARE traditional
zip.addText("legacy.txt", "data", NOTHING,
    {"password": "secret", "encryption_method": ZIP_EM_TRAD_PKWARE});

# Reading encrypted entries
ZipFile zip(encrypted_data);
zip.setPassword("secret");
string content = zip.readText("secret.txt");
```

### Verifying Integrity

```qore
ZipFile zip("archive.zip", "r");
list<hash<ZipVerifyResult>> results = zip.verify();
foreach hash<ZipVerifyResult> r in (results) {
    printf("%s: %s\n", r.name, r.valid ? "OK" : "FAIL: " + r.error);
}
```

### Replacing and Deleting Entries

```qore
# Replace one entry, delete another (unchanged entries are copied zero-copy)
ZipFile::replaceEntries("archive.zip",
    {"config.json": binary("{\"key\": \"new_value\"}")},
    ("temp.log",));
```

### Comparing Archives

```qore
hash<ZipDiffResult> diff = ZipFile::diff("v1.zip", "v2.zip");
printf("Added: %d, Deleted: %d, Modified: %d, Unchanged: %d\n",
    diff.added_count, diff.deleted_count, diff.modified_count, diff.unchanged_count);
```

### Re-compressing

```qore
# Convert from deflate to zstd
ZipFile::recompress("archive.zip", ZIP_CM_ZSTD, 3);
```

### Recovery Mode

```qore
# Open a corrupted archive using local file header recovery
ZipFile zip("damaged.zip", "r", True);  # recover=True
list<hash<ZipEntryInfo>> entries = zip.entries();
```

## Symlink Security

Archives can contain symlink entries that point outside the extraction directory (zip-slip
via symlink attack). By default, symlink entries are **silently skipped** during extraction.

To allow symlink extraction with target validation:

```qore
zip.extractAll("/dest", {"allow_symlinks": True});
```

Symlinks with absolute targets or targets that resolve outside the destination directory are
rejected with `ZIP-SECURITY-ERROR`.

## Data Provider API

The `ZipDataProvider` module exposes ZIP operations through Qore's data provider framework,
providing 13 actions with full UI metadata (display names, descriptions, allowed values).

### Actions

| Path | Action | Description |
|------|--------|-------------|
| `/archive/create` | Create Archive | Create a new archive from entries |
| `/archive/extract` | Extract Archive | Extract with filtering and path remapping |
| `/archive/list` | List Contents | List entries with optional filtering |
| `/archive/info` | Get Info | Archive metadata and statistics |
| `/archive/verify` | Verify Integrity | CRC32 verification without extracting |
| `/archive/add` | Add Files | Add entries to existing archive |
| `/archive/add-path` | Add Path | Recursively add file or directory |
| `/archive/replace` | Replace/Delete | Replace or delete entries |
| `/archive/recompress` | Recompress | Change compression method |
| `/archive/diff` | Diff Archives | Compare two archives |
| `/file/extract` | Extract File | Extract single file to memory or disk |
| `/data/compress` | Compress Data | Compress to in-memory archive |
| `/data/decompress` | Decompress Data | Decompress in-memory archive |

### Example

```qore
%requires ZipDataProvider

ZipDataProvider dp();
auto extractDp = dp.getChildProviderEx("archive").getChildProviderEx("extract");
hash<auto> result = extractDp.doRequest({
    "input_path": "archive.zip",
    "destination": "/output",
    "include_patterns": ("*.txt",),
    "exclude_patterns": ("temp_*",),
    "overwrite": True,
});
printf("Extracted %d files\n", result.entry_count);
```

## qzip CLI Tool

The `qzip` command-line tool provides colorized access to all module functionality.

### Commands

Commands support single-letter aliases for common operations:

| Alias | Command | Description |
|-------|---------|-------------|
| `c` | `qzip create [-c method] [-l level] <archive> <files>` | Create archive from files/directories |
| `x` | `qzip extract [-d dir] [-o] [-f] [-v] <archive>` | Extract with filtering and path options |
| `t` | `qzip list [-l] [-i glob] [-x glob] <archive>` | List entries with optional filtering |
| `a` | `qzip add [-c method] <archive> <files>` | Add files to existing archive |
| `i` | `qzip info [-i glob] <archive>` | Show archive metadata and statistics |
| `v` | `qzip verify [-i glob] <archive>` | Verify entry integrity (CRC32) |
| `d` | `qzip diff [-a] <archive1> <archive2>` | Compare two archives |
| `r` | `qzip recompress -c <method> [-l level] <archive>` | Re-compress with different method |
| | `qzip add-path [-r root] <archive> <path>` | Add directory recursively |
| | `qzip delete <archive> <entry> [entry...]` | Delete entries from archive |
| | `qzip replace <archive> <entry>=<file> [...]` | Replace entries with new content |

### Examples

```bash
# Create archive from directory
qzip create backup.zip /path/to/project/

# List with details (permissions, sizes, compression)
qzip list -l backup.zip

# Extract only source files
qzip extract -d /tmp/src -i "*.cpp" -i "*.h" backup.zip

# Verify integrity
qzip verify backup.zip

# Compare two versions
qzip diff v1.zip v2.zip

# Re-compress with zstd
qzip recompress -c zstd -l 3 backup.zip
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Copyright

Copyright 2026 Qore Technologies, s.r.o.
