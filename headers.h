/*
 *	headers.h
 *
 *	include headers
 */

#ifdef	__HOST_CC__
#include "hmachine.h"
#else
#include "machine.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef	NOUNISTDH
#include <unistd.h>
#endif

#ifndef	NOSTDLIBH
#include <stdlib.h>
#endif

#ifdef	USETIMEH
#include <time.h>
#endif

#ifdef	USESTDARGH
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/* for glibc */
#if	defined (LINUX) && defined (_SYS_SYSMACROS_H)
#include <sys/sysmacros.h>
#endif

#if	MSDOS
#undef	MAXPATHLEN
#define	MAXPATHLEN		260
#undef	MAXNAMLEN
#define	MAXNAMLEN		255
#else	/* !MSDOS */
#include <sys/time.h>
#include <sys/param.h>
#include <sys/file.h>
# ifdef	USEUTIME
# include <utime.h>
# endif
# ifdef	MINIX
# include <limits.h>
# endif
# if	!defined (MAXPATHLEN) && defined (PATH_MAX)
# define	MAXPATHLEN		PATH_MAX
# endif
#endif	/* !MSDOS */

#ifndef	O_BINARY
#define	O_BINARY		0
#endif
#ifndef	O_TEXT
#define	O_TEXT			0
#endif
#ifndef	O_NOCTTY
#define	O_NOCTTY		0
#endif
#ifndef	O_ACCMODE
#define	O_ACCMODE		(O_RDONLY | O_WRONLY | O_RDWR)
#endif

#ifndef	L_SET
# ifdef	SEEK_SET
# define	L_SET		SEEK_SET
# else
# define	L_SET		0
# endif
#endif	/* !L_SET */
#ifndef	L_INCR
# ifdef	SEEK_CUR
# define	L_INCR		SEEK_CUR
# else
# define	L_INCR		1
# endif
#endif	/* !L_INCR */
#ifndef	L_XTND
# ifdef	SEEK_END
# define	L_XTND		SEEK_END
# else
# define	L_XTND		2
# endif
#endif	/* !L_XTND */

#ifdef	__TURBOC__
#undef	EFAULT
#undef	ENFILE
#undef	ENOSPC
#undef	EROFS
#undef	EPERM
#undef	EINTR
#undef	EIO
#undef	ENXIO
#undef	ENOTDIR
#undef	EISDIR
#endif	/* __TURBOC__ */

#ifndef	ENOSPC
#define	ENOSPC			EACCES
#endif
#ifndef	ENODEV
#define	ENODEV			EACCES
#endif
#ifndef	EIO
#define	EIO			ENODEV
#endif
#ifndef	EISDIR
#define	EISDIR			EACCES
#endif
#ifndef	ENOTEMPTY
# ifdef	ENFSNOTEMPTY
# define	ENOTEMPTY	ENFSNOTEMPTY
# else
# define	ENOTEMPTY	EACCES
# endif
#endif	/* !ENOTEMPTY */
#ifndef	EPERM
#define	EPERM			EACCES
#endif
#ifndef	EFAULT
#define	EFAULT			ENODEV
#endif
#ifndef	EROFS
#define	EROFS			EACCES
#endif
#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG		ERANGE
#endif
#ifndef	ENOTDIR
#define	ENOTDIR			ENOENT
#endif
#ifndef	EINTR
#define	EINTR			0
#endif
#ifndef	ETIMEDOUT
#define	ETIMEDOUT		EIO
#endif

#ifdef	NOERRNO
extern int errno;
#endif
#ifdef	USESTRERROR
#define	Xstrerror		strerror
#else	/* !USESTRERROR */
# ifndef	DECLERRLIST
extern CONST char *CONST sys_errlist[];
# endif
#define	Xstrerror(n)		(char *)sys_errlist[n]
#endif	/* !USESTRERROR */

#ifdef	USESTDARGH
#define	VA_START(a, f)		va_start(a, f)
#else
#define	VA_START(a, f)		va_start(a)
#endif

#ifndef	R_OK
#define	R_OK			4
#endif
#ifndef	W_OK
#define	W_OK			2
#endif
#ifndef	X_OK
#define	X_OK			1
#endif
#ifndef	F_OK
#define	F_OK			0
#endif

#ifndef	LOCK_SH
#define	LOCK_SH			1
#endif
#ifndef	LOCK_EX
#define	LOCK_EX			2
#endif
#ifndef	LOCK_NB
#define	LOCK_NB			4
#endif
#ifndef	LOCK_UN
#define	LOCK_UN			8
#endif

#if	!defined (NOFLAGS) && defined (UF_SETTABLE) && defined (SF_SETTABLE)
#define	HAVEFLAGS
#endif

#undef	S_IRUSR
#undef	S_IWUSR
#undef	S_IXUSR
#undef	S_IRGRP
#undef	S_IWGRP
#undef	S_IXGRP
#undef	S_IROTH
#undef	S_IWOTH
#undef	S_IXOTH
#define	S_IRUSR			00400
#define	S_IWUSR			00200
#define	S_IXUSR			00100
#define	S_IRGRP			00040
#define	S_IWGRP			00020
#define	S_IXGRP			00010
#define	S_IROTH			00004
#define	S_IWOTH			00002
#define	S_IXOTH			00001
#define	S_IREAD_ALL		(S_IRUSR | S_IRGRP | S_IROTH)
#define	S_IWRITE_ALL		(S_IWUSR | S_IWGRP | S_IWOTH)
#define	S_IEXEC_ALL		(S_IXUSR | S_IXGRP | S_IXOTH)

#undef	S_IFLNK
#undef	S_IFSOCK
#undef	S_IFIFO
#undef	S_ISUID
#undef	S_ISGID
#undef	S_ISVTX
#define	S_IFLNK			0120000
#define	S_IFSOCK		0140000
#define	S_IFIFO			0010000
#define	S_ISUID			0004000
#define	S_ISGID			0002000
#define	S_ISVTX			0001000

#ifdef	HAVEFLAGS
# ifndef	UF_NODUMP
# define	UF_NODUMP	0x00000001
# endif
# ifndef	UF_IMMUTABLE
# define	UF_IMMUTABLE	0x00000002
# endif
# ifndef	UF_APPEND
# define	UF_APPEND	0x00000004
# endif
# ifndef	UF_NOUNLINK
# define	UF_NOUNLINK	0x00000010
# endif
# ifndef	SF_ARCHIVED
# define	SF_ARCHIVED	0x00010000
# endif
# ifndef	SF_IMMUTABLE
# define	SF_IMMUTABLE	0x00020000
# endif
# ifndef	SF_APPEND
# define	SF_APPEND	0x00040000
# endif
# ifndef	SF_NOUNLINK
# define	SF_NOUNLINK	0x00080000
# endif
#endif	/* HAVEFLAGS */

#ifdef	NOUID_T
typedef u_short			uid_t;
typedef u_short			gid_t;
#endif
#ifdef	OLDARGINT
typedef int			u_id_t;
typedef int			g_id_t;
#define	convuid(u)		(((u) == (uid_t)-1) ? (u_id_t)-1 : (u_id_t)(u))
#define	convgid(g)		(((g) == (gid_t)-1) ? (g_id_t)-1 : (g_id_t)(g))
#else
typedef uid_t			u_id_t;
typedef gid_t			g_id_t;
#define	convuid(u)		(u)
#define	convgid(g)		(g)
#endif
