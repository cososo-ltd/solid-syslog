# Setting up LittleFS

## What you provide

A LittleFS checkout, a `struct lfs_config` carrying your block-device callbacks,
and a mounted `lfs_t`. Mount it before creating any file, and keep it mounted for
as long as the library holds one.

## Build

```cmake
set(SOLIDSYSLOG_PLATFORMS "LittleFs;<Network>;<OsPrimitives>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::LittleFs)
```

The placeholders are whichever platforms the [capability matrix](../index.md)
says fill the rest of what your build needs. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

Compile LittleFS's `lfs.c` and `lfs_util.c` into your target;
`SolidSyslog::LittleFs` carries the adapter's own sources.

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

**`block_cycles` has no usable default.** LittleFS asserts that it is not zero,
and `-1` disables dynamic wear levelling - which a store that rewrites records in
rotation needs. Set it to a few hundred.

**A mount is not the adapter's to make.** Keep the filesystem mounted for as long
as any file created from it exists. The adapter cannot tell that it was
unmounted: what a call then does is LittleFS's, and any call that answers an lfs
error is reported with that code, as every other failure is.

## When it does not work

Install an error handler before you start; [error severity](../../error-severity.md)
says what each level is telling you.
