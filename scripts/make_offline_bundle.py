#!/usr/bin/env python3
"""Turn a built offline documentation site into the zip attached to a release.

Runs inside the documentation toolchain container, because that is what built
the site and owns the files. Doing the packaging on the host instead means
writing into a directory the container created as root, which fails on a CI
runner exactly as it does locally.

Three jobs:

- **Drop 404.html.** MkDocs builds it with absolute ``site_url`` paths, because
  a 404 must resolve wherever the server chooses to serve it from. That makes it
  the one page in the site that cannot work from a file:// URI, and it has
  nothing to do in a bundle nobody serves.
- **Write MANIFEST.txt.** What the footer says is for the reader: the release
  this documents. What an auditor needs is the rest - the commit, the build
  date, the toolchain that produced it - and it belongs beside the pages rather
  than in them.
- **Write README.txt.** The reader who extracts the zip and looks at a folder
  never sees the in-page notice, so the same instruction is written where a file
  manager shows it.

Usage:
  make_offline_bundle.py <site-dir> <zip-path> --version V --commit SHA
                         [--tag TAG] [--image DIGEST] [--date ISO8601]
"""

import argparse
import datetime
import os
import sys
import zipfile

README = """SolidSyslog documentation - {version}

Open index.html in a browser to read this offline. Every page, image and
stylesheet is inside this folder; nothing is fetched from the network.

Search is the one exception. It runs in a browser worker that loads an index
file, and browsers refuse to do that for a page opened directly from disk. To
enable it, serve this folder instead:

    python3 -m http.server

then open http://localhost:8000 in a browser.

MANIFEST.txt records exactly which build this is.
"""

MANIFEST = """SolidSyslog documentation bundle

Version:   {version}
Tag:       {tag}
Commit:    {commit}
Built:     {date}
Toolchain: {image}

This bundle is the documentation as it stood at the commit above. It is
self-contained: opening index.html makes no network request.
"""


def write_text(path, text):
    with open(path, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write(text)


def archive(site, zip_path):
    """Zip the site so that extracting it yields one named folder, not loose files."""
    root = os.path.splitext(os.path.basename(zip_path))[0]
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as bundle:
        for directory, _, names in os.walk(site):
            for name in sorted(names):
                full = os.path.join(directory, name)
                inside = os.path.relpath(full, site)
                bundle.write(full, os.path.join(root, inside))
    return root


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument('site')
    parser.add_argument('zip_path')
    parser.add_argument('--version', required=True)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--tag', default='')
    parser.add_argument('--image', default='unrecorded')
    parser.add_argument('--date', default='')
    args = parser.parse_args(argv[1:])

    if not os.path.isdir(args.site):
        print(f'no such site directory: {args.site}', file=sys.stderr)
        return 2

    server_only = os.path.join(args.site, '404.html')
    if os.path.exists(server_only):
        os.remove(server_only)

    date = args.date or datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')
    fields = {
        'version': args.version,
        'tag': args.tag or f'v{args.version}',
        'commit': args.commit,
        'date': date,
        'image': args.image,
    }
    write_text(os.path.join(args.site, 'MANIFEST.txt'), MANIFEST.format(**fields))
    write_text(os.path.join(args.site, 'README.txt'), README.format(**fields))

    root = archive(args.site, args.zip_path)
    size = os.path.getsize(args.zip_path)
    print(f'{args.zip_path}: {size // 1024} KiB, extracting to {root}/')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
