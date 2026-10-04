/* A test stand-in for the VxWorks 6.4 base header.
 *
 * Supplies the subset of vxWorks.h that Platform/VxWorks64 relies on, named as
 * the public API reference names it. It deliberately depends on no SolidSyslog
 * header: the real one does not, and a stand-in that did would stop standing
 * in.
 */
#ifndef VXWORKS_H
#define VXWORKS_H

typedef int STATUS;
typedef unsigned long long UINT64;

/* An entry point, as taskSpawn takes one, which a caller casts its entry
 * function to. The real header leaves the parameter list unspecified; the
 * stand-in names it void, because the host builds reject a declaration without
 * a prototype. The cast compiles against either. */
typedef int (*FUNCPTR)(void);

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define OK 0
#define ERROR (-1)
#define WAIT_FOREVER (-1)
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#endif /* VXWORKS_H */
