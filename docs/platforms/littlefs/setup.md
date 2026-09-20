# Setting up LittleFS

## What you provide

A LittleFS checkout, a `struct lfs_config` carrying your block-device callbacks,
and a mounted `lfs_t`. Mount it before creating any file, and keep it mounted for
as long as the library holds one.

## Build

Compile LittleFS's `lfs.c` and `lfs_util.c` into your target and link
`SolidSyslog::LittleFs`, which carries the adapter's own sources.

LittleFS's headers are built upstream with a narrower warning set than this
library's: `lfs_util.h` does not compile under `-Wsign-conversion`. Add the tree
as a **system** include so its warnings do not reach your build, rather than
relaxing your own:

```cmake
target_include_directories(your_target SYSTEM PRIVATE ${LITTLEFS_PATH})
```

## Creating a file

```c
static uint8_t recordFileCache[LFS_CACHE_SIZE];

struct SolidSyslogFile* file =
    SolidSyslogLittleFsFile_Create(&lfs, recordFileCache, sizeof(recordFileCache));
```

The buffer must be at least the `cache_size` you mounted with, and must outlive
the file. It is the file's own cache, so give each file its own.

## What will catch you out

**The cache buffer is per file, not per filesystem.** LittleFS already has a read
cache and a program cache of its own; this is a third, and every open file needs
one. Creating two files from one buffer corrupts both, and nothing reports it -
the adapter cannot tell two callers apart.

**Getting the size wrong is refused, not tolerated.** A buffer smaller than
`cache_size` makes Create return the Null file and raise a `CRITICAL`. Size it
from the same constant you mount with rather than a number typed twice.

**`block_cycles` defaults to no wear levelling.** LittleFS disables dynamic wear
levelling unless you set it, and a store that rewrites records in rotation is
exactly the workload that needs it. Set it to a few hundred.

**A mount is not the adapter's to make.** If the filesystem is unmounted while a
file is open, every later call fails through LittleFS rather than through the
library, and the library has nothing to report.
