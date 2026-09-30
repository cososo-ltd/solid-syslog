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

#endif /* VXWORKS_H */
