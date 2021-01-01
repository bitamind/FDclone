/*
 *	unixdisk.h
 *
 *	definitions & function prototype declarations for "unixdisk.c"
 */

#if	MSDOS && !defined __UNIXDISK_H_
#define	__UNIXDISK_H_

#include "depend.h"
#include "termio.h"
#include "unixemu.h"
#include "fsinfo.h"

#ifdef	DJGPP
#define	NOP			0x00
#define	RETF			0xc3
#else
#define	NOP			0x90
#define	RETF			0xcb
#endif

#define	DATETIMEFORMAT		1
#define	DS_IRDONLY		001
#define	DS_IHIDDEN		002
#define	DS_IFSYSTEM		004
#define	DS_IFLABEL		010
#define	DS_IFDIR		020
#define	DS_IARCHIVE		040
#define	SEARCHATTRS		(DS_IRDONLY | DS_IHIDDEN | DS_IFSYSTEM \
				| DS_IFDIR | DS_IARCHIVE)
#define	PSEUDOINTNO		0x80
#define	MAXINTNO		0xff

#define	BOOTSECTSIZE		512
#define	MAXSECTSIZE		4096
#define	BIOSRETRY		4
#define	VOL_FAT32		"FAT32"

#ifdef	PC98
#define	DISKBIOS		0x1b
#define	BIOS_READ		0x06
#define	BIOS_WRITE		0x05
#define	BIOS_VERIFY		0x01
#define	BIOS_PARAM		0x84
#define	BIOS_RESET		0x03
#define	BIOS_HDD		0x80
#define	BIOS_SCSI		0xa0
#define	BIOS_DMAERR		0x20
#define	MAX_HDD			4
#define	MAX_SCSI		8

#define	PT_FAT12		0x81	/* 0x80 | 0x01 */
#define	PT_FAT16		0x91	/* 0x80 | 0x11 */
#define	PT_FREEBSD		0x94	/* 0x80 | 0x14 */
#define	PT_FAT16X		0xa1	/* 0x80 | 0x21 */
#define	PT_NTFS			0xb1	/* 0x80 | 0x31 */
#define	PT_386BSD		0xc4	/* 0x80 | 0x44 */
#define	PT_FAT32		0xe1	/* 0x80 | 0x61 */
#define	PT_LINUX		0xe2	/* 0x80 | 0x62 */

#define	PART_TABLE		0x0000
#define	PART_NUM		16
#else	/* !PC98 */
#define	DISKBIOS			0x13
#define	BIOS_READ		0x02
#define	BIOS_WRITE		0x03
#define	BIOS_VERIFY		0x04
#define	BIOS_PARAM		0x08
#define	BIOS_RESET		0x00
#define	BIOS_TYPE		0x15
#define	BIOS_XCHECK		0x41
#define	BIOS_XREAD		0x42
#define	BIOS_XWRITE		0x43
#define	BIOS_XVERIFY		0x44
#define	BIOS_XPARAM		0x48
#define	BIOS_HDD		0x80
#define	BIOS_DMAERR		0x09

#define	DT_NODRIVE		0x00
#define	DT_FLOPPY		0x01
#define	DT_FLOPPY_CL		0x02
#define	DT_HARDDISK		0x03

#define	PT_FAT12		0x01
#define	PT_FAT16		0x04
#define	PT_EXTEND		0x05
#define	PT_FAT16X		0x06
#define	PT_NTFS			0x07
#define	PT_FAT32		0x0b
#define	PT_FAT32LBA		0x0c
#define	PT_FAT16XLBA		0x0e
#define	PT_EXTENDLBA		0x0f
#define	PT_LINUX		0x83
#define	PT_386BSD		0xa5
#define	PT_OPENBSD		0xa6
#define	PT_NETBSD		0xa9

#define	PART_TABLE		0x01be
#define	PART_NUM		4
#endif	/* !PC98 */

typedef struct _partition_t {
	u_char boot;
#ifdef	PC98
	u_char filesys;
	u_char reserved[2];
	u_char ipl_sect;
	u_char ipl_head;
	u_char ipl_cyl[2];
	u_char s_sect;
	u_char s_head;
	u_char s_cyl[2];
	u_char e_sect;
	u_char e_head;
	u_char e_cyl[2];
	u_char name[16];
#else
	u_char s_head;
	u_char s_sect;
	u_char s_cyl;
	u_char filesys;
	u_char e_head;
	u_char e_sect;
	u_char e_cyl;
	u_char f_sect[4];
	u_char t_sect[4];
#endif
} X_attr_packed partition_t;
#define	PART_SIZE	((int)sizeof(partition_t))

typedef struct _xparam_t {
	u_char size[2];
	u_char flags[2];
	u_char cyl[4];
	u_char head[4];
	u_char sect[4];
	u_char total[8];
	u_char sectsize[2];
	u_char eddparam[4];
	u_char signature[2];
	u_char infolen;
	u_char reserved[3];
	u_char busname[4];
	u_char interfacename[8];
	u_char interfacepath[8];
	u_char devicepath[8];
	u_char reserved2;
	u_char checksum;
} X_attr_packed xparam_t;
#define	XPARAM_SIZE	((int)sizeof(xparam_t))

typedef struct _xpacket_t {
	u_char size;
	u_char reserved;
	u_char nsect[2];
	u_char bufptr[4];
	u_char sect[8];
} X_attr_packed xpacket_t;
#define	XPACKET_SIZE	((int)sizeof(xpacket_t))

typedef struct _drvinfo {
	u_long head;
	u_long sect;
	u_long cyl;
	u_long secthead;
	u_char s_head;
	u_char s_sect;
	u_short s_cyl;
#ifndef	PC98
	u_long f_sect;
#endif
	u_short sectsize;
	u_char drv;
	u_char flags;
	u_char filesys;
} drvinfo;

#define	DI_TYPE			0003
#define	DI_NOPLOVED		0000
#define	DI_FIXED		0001
#define	DI_REMOVABLE		0002
#define	DI_MISC			0003
#define	DI_PSEUDO		0004
#define	DI_CHECKED		0010
#define	DI_LBA			0020
#define	DI_INVALIDCHS		0040

struct dosfind_t {
	u_char keyattr;
	u_char drive;
	char body[8], ext[3];
	char reserve[8];
	u_char attr;
	u_short wrtime, wrdate;
	u_long size_l;
	char name[13];
} X_attr_packed;

struct lfnfind_t {
	u_long attr;
	u_short crtime, crdate, crtime_h1, crtime_h2;
	u_short actime, acdate, actime_h1, actime_h2;
	u_short wrtime, wrdate, wrtime_h1, wrtime_h2;
	u_long size_h, size_l;
	u_long reserve1, reserve2;
	char name[MAXPATHLEN];
	char alias[14];
} X_attr_packed;

struct iopacket_t {
	u_long sect;
	u_short size;
	u_short buf_off;
	u_short buf_seg;
} X_attr_packed;

struct fat32statfs_t {
	u_short f_type;
	u_short f_version;
	u_long f_clustsize;
	u_long f_sectsize;
	u_long f_bavail;
	u_long f_blocks;
	u_long f_real_bavail_sect;
	u_long f_real_blocks_sect;
	u_long f_real_bavail;
	u_long f_real_blocks;
	u_char reserved[8];
} X_attr_packed;

extern int getcurdrv __P_((VOID_A));
extern int setcurdrv __P_((int, int));
#ifndef	_NOUSELFN
extern int getdosver __P_((VOID_A));
extern int supportLFN __P_((CONST char *));
#endif
extern char *unixgetcurdir __P_((char *, int));
#ifndef	_NOUSELFN
extern char *shortname __P_((CONST char *, char *));
#endif
extern char *unixrealpath __P_((CONST char *, char *));
#ifndef	BSPATHDELIM
extern char *adjustpname __P_((char *));
#endif
#if	defined (DJGPP) || !defined (BSPATHDELIM)
extern char *adjustfname __P_((char *));
#endif
#ifndef	_NOUSELFN
extern char *preparefile __P_((CONST char *, char *));
# ifndef	_NODOSDRIVE
extern int checkdrive __P_((int));
extern int rawdiskio __P_((int, u_long, u_char *, int, int, int));
# endif
#endif	/* !_NOUSELFN */
extern DIR *unixopendir __P_((CONST char *));
extern int unixclosedir __P_((DIR *));
extern struct dirent *unixreaddir __P_((DIR *));
extern int unixrewinddir __P_((DIR *));
#ifdef	_NOUSELFN
#define	unixunlink(p)		((unlink(p)) ? -1 : 0)
#define	unixrename(f, t)	((rename(f, t)) ? -1 : 0)
# ifdef	DJGPP
# define	unixmkdir(p, m)	((mkdir(p, m)) ? -1 : 0)
# else
extern int unixmkdir __P_((CONST char *, int));
# endif
#define	unixrmdir(p)		((rmdir(p)) ? -1 : 0)
#define	unixchdir(p)		((chdir(p)) ? -1 : 0)
#else
extern int unixunlink __P_((CONST char *));
extern int unixrename __P_((CONST char *, CONST char *));
extern int unixmkdir __P_((CONST char *, int));
extern int unixrmdir __P_((CONST char *));
extern int unixchdir __P_((CONST char *));
#endif
extern char *unixgetcwd __P_((char *, int));
extern int unixstatfs __P_((CONST char *, statfs_t *));
extern int unixstat __P_((CONST char *, struct stat *));
extern int unixchmod __P_((CONST char *, int));
#ifdef	_NOUSELFN
#define	unixutimes		rawutimes
#else	/* !_NOUSELFN */
extern int unixutimes __P_((CONST char *, CONST struct utimes_t *));
extern int unixopen __P_((CONST char *, int, int));
#endif	/* !_NOUSELFN */

#endif	/* MSDOS && !__UNIXDISK_H_ */
