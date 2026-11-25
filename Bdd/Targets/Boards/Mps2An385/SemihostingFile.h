#ifndef SEMIHOSTING_FILE_H
#define SEMIHOSTING_FILE_H

/* A file on the QEMU host, reached over ARM semihosting (BKPT 0xAB). The
 * machine has no storage of its own, so whatever it keeps from one run to the
 * next is such a file, in QEMU's working directory. */

/* Opens the file of that name where it is at least that many bytes long, and
 * otherwise creates it at that length, every byte of it reading as zero.
 * Answers its handle, or a negative value if it could do neither. */
int SemihostingFile_OpenImage(const char* path, int bytes);

/* Reads or writes that many bytes at that position. Zero for success. */
int SemihostingFile_ReadAt(int handle, int position, void* buffer, int count);
int SemihostingFile_WriteAt(int handle, int position, const void* buffer, int count);

#endif /* SEMIHOSTING_FILE_H */
