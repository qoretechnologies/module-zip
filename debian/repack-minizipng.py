#!/usr/bin/python3
# Copyright (C) 2026 David Nichols
# SPDX-License-Identifier: MIT
"""Create the offline minizipng orig component from the pinned upstream archive."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('upstream_archive', type=Path)
parser.add_argument('output_directory', type=Path)
args = parser.parse_args()
debian = Path(__file__).resolve().parent
spec = json.loads((debian / 'vendor-sources.json').read_text())['minizipng']
actual = hashlib.sha256(args.upstream_archive.read_bytes()).hexdigest()
if actual != spec['sha256']:
    raise SystemExit(f'minizipng checksum mismatch: expected {spec["sha256"]}, got {actual}')
def field(name):
    return subprocess.check_output(['dpkg-parsechangelog', '-l' + str(debian / 'changelog'),
                                    '-S' + name], text=True).strip()
version = field('Version').rsplit('-', 1)[0]
epoch = int(field('Timestamp'))
output = args.output_directory / f'qore-zip-module_{version}.orig-minizipng.tar.xz'
args.output_directory.mkdir(parents=True, exist_ok=True)
# Refuse to replace a source archive already used for qualification or signing.
with output.open('xb') as stream, tarfile.open(args.upstream_archive) as original:
    with tarfile.open(fileobj=stream, mode='w:xz', format=tarfile.GNU_FORMAT) as repacked:
        for item in sorted(original.getmembers(), key=lambda entry: entry.name):
            path = PurePosixPath(item.name)
            if path.is_absolute() or '..' in path.parts or path.parts[0] != 'minizip-ng-' + spec['version']:
                raise SystemExit(f'Unsafe upstream archive path: {item.name}')
            if len(path.parts) > 1 and path.parts[1] in spec['excluded']:
                continue
            if not (item.isfile() or item.isdir()):
                raise SystemExit(f'Unsupported upstream archive entry: {item.name}')
            item.uid = item.gid = 0
            item.uname = item.gname = 'root'
            item.mtime = epoch
            repacked.addfile(item, original.extractfile(item) if item.isfile() else None)
print(output)
