/*
 *	string.h
 *
 *	function prototype declarations for "string.c"
 */

#ifdef	MINIMUMSHELL
# ifndef	_NOXMEMCHR
# define	_NOXMEMCHR
# endif
# ifndef	_NOXSTRCAT
# define	_NOXSTRCAT
# endif
#endif	/* MINIMUMSHELL */
#if	(!defined (FD) || defined (_NOVERSCMP)) && !defined (_NOXSTRVERSCMP)
#define	_NOXSTRVERSCMP
#endif
#if	(!MSDOS \
&& ((!defined (FD) && !defined (WITHNETWORK)) || defined (_NODOSCOMMAND))) \
&& !defined (_NOXSTRTOLOWER)
#define	_NOXSTRTOLOWER
#endif

#ifdef	_NOXSTRCHR
#define	Xstrchr			strchr
#define	Xstrrchr		strrchr
#else
extern char *Xstrchr __P_((CONST char *, int));
extern char *Xstrrchr __P_((CONST char *, int));
#endif
#ifdef	_NOXMEMCHR
#define	Xmemchr			memchr
#else
extern char *Xmemchr __P_((CONST char *, int, int));
#endif
#ifndef	_NOXSTRCPY
extern char *Xstrcpy __P_((char *, CONST char *));
extern char *Xstrncpy __P_((char *, CONST char *, int));
#endif
#ifdef	_NOXSTRCAT
#define	Xstrcat			strcat
#define	Xstrlcat(s1, s2, n)	strcat(s1, s2)
#else
#define	Xstrcat(s1, s2)		Xstrlcat(s1, s2, sizeof(s1) - 1)
extern char *Xstrlcat __P_((char *, CONST char *, int));
#endif
#ifdef	_NOXSTRCASECMP
#define	Xstrcasecmp		strcasecmp
#define	Xstrncasecmp		strncasecmp
#else
extern int Xstrcasecmp __P_((CONST char *, CONST char *));
extern int Xstrncasecmp __P_((CONST char *, CONST char *, int));
#endif
#ifndef	_NOXSTRVERSCMP
extern int Xstrverscmp __P_((CONST char *, CONST char *, int));
#endif
#if	defined (_NOSTRLEN2) || !defined (CODEEUC)
#define	strlen2			strlen
#else
extern int strlen2 __P_((CONST char *));
#endif
#ifndef	_NOXSTRTOLOWER
extern VOID Xstrtolower __P_((char *));
extern VOID Xstrtoupper __P_((char *));
extern VOID Xstrntolower __P_((char *, int));
extern VOID Xstrntoupper __P_((char *, int));
#endif	/* !_NOXSTRTOLOWER */
