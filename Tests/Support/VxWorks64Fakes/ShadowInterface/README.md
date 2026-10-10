# VxWorks 6.4 stand-ins that shadow host headers

Some VxWorks 6.4 headers, such as `time.h` and `sys/stat.h`, share their name
with a host header but declare a different API. The stand-ins for those live
here rather than in `../Interface`, because on the include path of a test
executable they would replace the host's header for the test framework and the
C library too.

## Using one

- Only the VxWorks C sources that call the API see this directory: list them
  under `SHADOWED` in `vxworks64_add_test` (`Tests/VxWorks64/CMakeLists.txt`),
  which puts it on those sources' include path alone, ahead of the host's
  headers.
- A C++ test never includes a stand-in from here. It drives the fake through
  its `VxWorks64*Fake.h` in `../Interface`, in types the host can compile.
- A shadowed source includes `vxWorks.h`, the shadowed headers and as little
  else as it can. A host header it includes that itself includes, say,
  `time.h` gets the stand-in instead; on MSVC that breaks the build.

## When the names clash at link time

The host C library, the test framework and the sanitizer runtimes call some of
these functions themselves. A fake defined under the real name would be handed
to them as well. Instead the stand-in renames each such function with a macro
to a fake symbol:

```c
#define clock_gettime VxWorks64ClockFake_ClockGettime
```

The shadowed sources compile against the fake name, the fake defines it, and
nothing else in the link sees it. The pack itself compiles against the real
header unchanged.
