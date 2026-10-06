/* A test stand-in for the VxWorks 6.4 file status header.
 *
 * Supplies the subset of sys/stat.h that the VxWorks 6.4 BDD target calls,
 * with the kernel prototypes. It shadows the host's sys/stat.h, and renames
 * stat - the function and, with it, the struct tag - to the file-system
 * fake's, as ../README.md describes. Relies on vxWorks.h having been included
 * first, as VxWorks does.
 */
#ifndef SYS_STAT_H
#define SYS_STAT_H

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define S_IFMT 0xf000
#define S_IFDIR 0x4000
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#define stat VxWorks64FsFake_Stat

typedef int mode_t;

struct stat
{
    mode_t st_mode;
};

STATUS stat(const char* _name, struct stat* _pStat);

#endif /* SYS_STAT_H */
