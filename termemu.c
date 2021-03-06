/*
 *	termemu.c
 *
 *	terminal emulation
 */

#include "fd.h"
#include "wait.h"
#include "termio.h"
#include "func.h"
#include "kanji.h"
#include "system.h"
#include "termemu.h"

#ifdef	DEP_PTY

extern char fullpath[];
extern int hideclock;
extern int fdmode;
#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
extern jobtable *joblist;
extern int maxjobs;
#endif

int ptymode = 0;
int ptyinternal = 0;
char *ptyterm = NULL;
int ptymenukey = -1;
ptyinfo_t ptylist[MAXWINDOWS];
p_id_t emupid = (p_id_t)0;
int emufd = -1;
int parentfd = -1;
char *ptytmpfile = NULL;

static VOID NEAR doscroll __P_((int, int, int, int));
#ifdef	CYGWIN
static VOID NEAR closeallpty __P_((VOID_A));
#endif
static int NEAR genbackend __P_((VOID_A));
static VOID NEAR sendvar __P_((int, char **));
#ifdef	DEP_ORIGSHELL
static VOID NEAR sendheredoc __P_((int, heredoc_t *));
static VOID NEAR sendrlist __P_((int, redirectlist *));
static VOID NEAR sendcommand __P_((int, command_t *, syntaxtree *));
static VOID NEAR sendstree __P_((int, syntaxtree *));
#endif
static VOID NEAR awakechild __P_((CONST char *, CONST char *, int));
#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
static int trap_hup __P_((VOID_A));
static int NEAR recvmacro __P_((char **, char **, int *));
#endif
static int NEAR callmacro __P_((CONST char *, CONST char *, int));


static VOID NEAR doscroll(n, c, x, y)
int n, c, x, y;
{
	Xlocate(x, y);
	while (c--) Xputterms(n);
}

VOID regionscroll(n, c, x, y, min, max)
int n, c, x, y, min, max;
{
	int y1, y2;

	if (min < 0) min = 0;
	if (max < 0) max = n_line - 1;
	if (min <= 0 && max >= n_line - 1) {
		doscroll(n, c, x, y);
		return;
	}

	if (Xsetscroll(min, max) >= 0) {
		doscroll(n, c, x, y);
		Xsetscroll(0, n_line - 1);
		Xlocate(x, y);
		return;
	}

	y1 = y2 = -1;
	switch (n) {
		case C_DOWN:
		case C_SCROLLFORW:
		case C_NEWLINE:
			if (y != max) break;
			y1 = min;
			if (max < n_line - 1) y2 = max - c + 1;
			break;
		case C_UP:
		case C_SCROLLREV:
			if (y != min) break;
			if (max < n_line - 1) y1 = max - c + 1;
			y2 = min;
			break;
		case L_DELETE:
			if (max >= n_line - 1) break;
			y1 = y;
			y2 = max - c + 1;
			break;
		case L_INSERT:
			if (max >= n_line - 1) break;
			y1 = max - c + 1;
			y2 = y;
			break;
		default:
			break;
	}

	if (y1 < 0 && y2 < 0) doscroll(n, c, x, y);
	else {
		n = (y1 < y2) ? y1 : y2;
		if (c > max - n + 1) c = max - n + 1;
		if (y1 > 0) doscroll(L_DELETE, c, 0, y1);
		if (y2 > 0) doscroll(L_INSERT, c, 0, y2);
		Xlocate(x, y);
	}
}

int selectpty(max, fds, result, timeout)
int max, fds[];
char result[];
long timeout;
{
	struct timeval tv, *t;

	if (timeout < 0L) t = NULL;
	else if (!timeout) {
		tv.tv_sec = 0L;
		tv.tv_usec = 1L;
		t = &tv;
	}
	else {
		tv.tv_sec = timeout / 1000000L;
		tv.tv_usec = timeout % 1000000L;
		t = &tv;
	}

	return(readselect(max, fds, result, t));
}

#ifdef	CYGWIN
static VOID NEAR closeallpty(VOID_A)
{
	int i;

	for (i = 0; i < MAXWINDOWS; i++) {
		safeclose(ptylist[i].fd);
		ptylist[i].fd = -1;
	}
}
#endif	/* CYGWIN */

static int NEAR genbackend(VOID_A)
{
	char path[MAXPATHLEN];
	p_id_t pid;
	int i, n, fds[2], slave[MAXWINDOWS];

	if (emupid) return(0);

	if (pipe(fds) < 0) return(-1);
	VOID_C fcntl(fds[0], F_SETFL, O_NONBLOCK);
	VOID_C fcntl(fds[1], F_SETFL, O_NONBLOCK);

	for (i = 0; i < MAXWINDOWS; i++) {
		n = Xopenpty(&(ptylist[i].fd),
			&(slave[i]), path, sizeof(path));
		if (n < 0) break;
		ptylist[i].pid = (p_id_t)0;
		ptylist[i].path = Xstrdup(path);
		ptylist[i].pipe = -1;
		ptylist[i].status = 0;
	}

	if (i < MAXWINDOWS || (pid = Xfork()) < (p_id_t)0) {
		safeclose(fds[0]);
		safeclose(fds[1]);
		while (--i >= 0) {
			safeclose(ptylist[i].fd);
			ptylist[i].fd = -1;
			safeclose(slave[i]);
			Xfree(ptylist[i].path);
			ptylist[i].path = NULL;
		}
		return(-1);
	}

	if (!pid) {
#ifdef	DEP_ORIGSHELL
		mypid = getpid();
#endif
		safeclose(fds[1]);
		emufd = fds[0];
		emupid = (p_id_t)0;
		for (i = 0; i < MAXWINDOWS; i++) {
			Xfree(ptylist[i].path);
			ptylist[i].path = NULL;
			resetptyterm(i, 1);
		}
		resetptyterm(i, 1);

		backend();
		for (i = 0; i < MAXWINDOWS; i++) {
			safeclose(ptylist[i].fd);
			ptylist[i].fd = -1;
			safeclose(slave[i]);
		}
		_exit(0);
	}

	safeclose(fds[0]);
	emufd = newdup(fds[1]);
	emupid = pid;
	for (i = 0; i < MAXWINDOWS; i++) {
#ifdef	CYGWIN
		ptylist[i].fd = newdup(ptylist[i].fd);
#else
		safeclose(ptylist[i].fd);
		ptylist[i].fd = -1;
#endif
		safeclose(slave[i]);
	}

	return(0);
}

VOID syncptyout(fd, cmd)
int fd, cmd;
{
	char *tty;
	u_char uc;

	if (parentfd < 0) return;

	savetermio(ttyio, &tty, NULL);
	keyflush();
	Xnoecho();
	if (fd < 0) {
		tputs2("\033[99n", 1);
		tflush();
	}
	else {
		uc = cmd;
		sendbuf(fd, &uc, sizeof(uc));
	}
	VOID_C Xgetch();
	keyflush();
	loadtermio(ttyio, tty, NULL);
	Xfree(tty);
}

int recvbuf(fd, buf, nbytes)
int fd;
VOID_P buf;
int nbytes;
{
	char *cp;
	int n;

	cp = (char *)buf;
	for (;;) {
		if ((n = sureread(fd, cp, nbytes)) >= nbytes) {
			errno = 0;
			return(0);
		}
		else if (n <= 0) break;
		cp += n;
		nbytes -= n;
	}

	return(-1);
}

VOID sendbuf(fd, buf, nbytes)
int fd;
CONST VOID_P buf;
int nbytes;
{
	VOID_C surewrite(fd, buf, nbytes);
}

int recvword(fd, np)
int fd, *np;
{
	short w;

	if (recvbuf(fd, &w, sizeof(w)) < 0) return(-1);
	*np = (int)w;

	return(0);
}

VOID sendword(fd, n)
int fd, n;
{
	short w;

	w = (short)n;
	sendbuf(fd, &w, sizeof(w));
}

int recvstring(fd, cpp)
int fd;
char **cpp;
{
	char *cp;
	ALLOC_T len;

	if (recvbuf(fd, &cp, sizeof(cp)) < 0) return(-1);
	if (cp) {
		if (recvbuf(fd, &len, sizeof(len)) < 0) return(-1);
		cp = Xmalloc(len + 1);
		if (recvbuf(fd, cp, len) < 0) {
			Xfree(cp);
			return(-1);
		}
		cp[len] = '\0';
	}
	*cpp = cp;

	return(0);
}

VOID sendstring(fd, s)
int fd;
CONST char *s;
{
	ALLOC_T len;

	sendbuf(fd, &s, sizeof(s));
	if (!s) return;

	len = strlen(s);
	sendbuf(fd, &len, sizeof(len));
	if (len) sendbuf(fd, s, len);
}

static VOID NEAR sendvar(fd, var)
int fd;
char **var;
{
	int i, c;

	sendbuf(fd, &var, sizeof(var));
	if (!var) return;

	c = countvar(var);
	sendbuf(fd, &c, sizeof(c));
	for (i = 0; i < c; i++) sendstring(fd, var[i]);
}

#ifdef	DEP_ORIGSHELL
static VOID NEAR sendheredoc(fd, hdp)
int fd;
heredoc_t *hdp;
{
	sendbuf(fd, &hdp, sizeof(hdp));
	if (!hdp) return;

	sendbuf(fd, hdp, sizeof(*hdp));
	sendstring(fd, hdp -> eof);
	sendstring(fd, hdp -> filename);
	sendstring(fd, hdp -> buf);
}

static VOID NEAR sendrlist(fd, rp)
int fd;
redirectlist *rp;
{
	sendbuf(fd, &rp, sizeof(rp));
	if (!rp) return;

	sendbuf(fd, rp, sizeof(*rp));
	if (rp -> type & MD_HEREDOC)
		sendheredoc(fd, (heredoc_t *)(rp -> filename));
	else sendstring(fd, rp -> filename);
	sendrlist(fd, rp -> next);
}

static VOID NEAR sendcommand(fd, comm, trp)
int fd;
command_t *comm;
syntaxtree *trp;
{
	sendbuf(fd, &comm, sizeof(comm));
	if (!comm) return;

	if (trp -> flags & ST_NODE) {
		sendstree(fd, (syntaxtree *)comm);
		return;
	}

	sendbuf(fd, comm, sizeof(*comm));
	if (!isstatement(comm)) sendvar(fd, comm -> argv);
	else sendstree(fd, (syntaxtree *)(comm -> argv));
	sendrlist(fd, comm -> redp);
}

static VOID NEAR sendstree(fd, trp)
int fd;
syntaxtree *trp;
{
	sendbuf(fd, &trp, sizeof(trp));
	if (!trp) return;

	sendbuf(fd, trp, sizeof(*trp));
	sendcommand(fd, trp -> comm, trp);
	sendstree(fd, trp -> next);
}
#endif	/* DEP_ORIGSHELL */

#ifdef	USESTDARGH
/*VARARGS1*/
VOID sendparent(int cmd, ...)
#else
/*VARARGS1*/
VOID sendparent(cmd, va_alist)
int cmd;
va_dcl
#endif
{
#ifdef	DEP_ORIGSHELL
	syntaxtree *trp;
#endif
#ifndef	_NOARCHIVE
	lsparse_t *lp;
	archive_t *ap;
#endif
#ifdef	DEP_DOSEMU
	devinfo *devp;
#endif
	bindtable *bindp;
	keyseq_t *keyp;
	va_list args;
	char *cp, *func1, *func2, ***varp, **var;
	u_char uc;
	int n, fd, val;

	if (parentfd < 0) return;
#ifdef	DEP_ORIGSHELL
	if (mypid != shellpid) return;
#endif

	fd = parentfd;
	syncptyout(fd, TE_LOCKFRONT);
	uc = cmd;
	sendbuf(fd, &uc, sizeof(uc));

	VA_START(args, cmd);
	switch (cmd) {
		case TE_SETVAR:
			varp = va_arg(args, char ***);
			var = va_arg(args, char **);
			sendbuf(fd, &varp, sizeof(varp));
			sendvar(fd, var);
			break;
		case TE_PUSHVAR:
			varp = va_arg(args, char ***);
			cp = va_arg(args, char *);
			sendbuf(fd, &varp, sizeof(varp));
			sendstring(fd, cp);
			break;
		case TE_POPVAR:
			varp = va_arg(args, char ***);
			sendbuf(fd, &varp, sizeof(varp));
			break;
		case TE_CHDIR:
#ifdef	DEP_ORIGSHELL
		case TE_SETEXPORT:
		case TE_SETRONLY:
#endif
#if	!defined (DEP_ORIGSHELL) || !defined (NOALIAS)
		case TE_DELETEALIAS:
#endif
#ifndef	DEP_ORIGSHELL
		case TE_DELETEFUNCTION:
#endif
			cp = va_arg(args, char *);
			sendstring(fd, cp);
			break;
#ifdef	DEP_ORIGSHELL
		case TE_PUTEXPORTVAR:
		case TE_PUTSHELLVAR:
		case TE_UNSET:
		case TE_DELETEFUNCTION:
			cp = va_arg(args, char *);
			n = va_arg(args, int);
			sendbuf(fd, &n, sizeof(n));
			sendstring(fd, cp);
			break;
		case TE_SETSHFLAG:
			n = va_arg(args, int);
			val = va_arg(args, int);
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, &val, sizeof(val));
			break;
		case TE_ADDFUNCTION:
			cp = va_arg(args, char *);
			trp = va_arg(args, syntaxtree *);
			sendstring(fd, cp);
			sendstree(fd, trp);
			break;
#else	/* !DEP_ORIGSHELL */
		case TE_PUTSHELLVAR:
			cp = va_arg(args, char *);
			func1 = va_arg(args, char *);
			n = va_arg(args, int);
			sendbuf(fd, &n, sizeof(n));
			sendstring(fd, func1);
			sendstring(fd, cp);
			break;
		case TE_ADDFUNCTION:
			cp = va_arg(args, char *);
			var = va_arg(args, char **);
			sendvar(fd, var);
			sendstring(fd, cp);
			break;
#endif	/* !DEP_ORIGSHELL */
#if	!defined (DEP_ORIGSHELL) || !defined (NOALIAS)
		case TE_ADDALIAS:
			cp = va_arg(args, char *);
			func1 = va_arg(args, char *);
			sendstring(fd, func1);
			sendstring(fd, cp);
			break;
#endif
		case TE_SETHISTORY:
			cp = va_arg(args, char *);
			n = va_arg(args, int);
			sendbuf(fd, &n, sizeof(n));
			sendstring(fd, cp);
			break;
		case TE_ADDKEYBIND:
			n = va_arg(args, int);
			bindp = va_arg(args, bindtable *);
			func1 = va_arg(args, char *);
			func2 = va_arg(args, char *);
			cp = va_arg(args, char *);
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, bindp, sizeof(*bindp));
			sendstring(fd, func1);
			sendstring(fd, func2);
			sendstring(fd, cp);
			break;
		case TE_DELETEKEYBIND:
#ifndef	_NOARCHIVE
		case TE_DELETELAUNCH:
		case TE_DELETEARCH:
#endif
#ifdef	DEP_DOSEMU
		case TE_DELETEDRV:
#endif
#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
		case TE_CHANGESTATUS:
#endif
#ifdef	DEP_IME
		case TE_FREEROMAN:
#endif
			n = va_arg(args, int);
			sendbuf(fd, &n, sizeof(n));
			break;
		case TE_SETKEYSEQ:
			keyp = va_arg(args, keyseq_t *);
			sendbuf(fd, &(keyp -> code), sizeof(keyp -> code));
			sendbuf(fd, &(keyp -> len), sizeof(keyp -> len));
			sendstring(fd, keyp -> str);
			break;
#ifndef	_NOARCHIVE
		case TE_ADDLAUNCH:
			n = va_arg(args, int);
			lp = va_arg(args, lsparse_t *);
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, lp, sizeof(*lp));
			sendstring(fd, lp -> ext);
			sendstring(fd, lp -> comm);
# ifndef	OLDPARSE
			sendvar(fd, lp -> format);
			sendvar(fd, lp -> lignore);
			sendvar(fd, lp -> lerror);
# endif
			break;
		case TE_ADDARCH:
			n = va_arg(args, int);
			ap = va_arg(args, archive_t *);
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, &(ap -> flags), sizeof(ap -> flags));
			sendstring(fd, ap -> ext);
			sendstring(fd, ap -> p_comm);
			sendstring(fd, ap -> u_comm);
			break;
#endif	/* !_NOARCHIVE */
#ifdef	DEP_DOSEMU
		case TE_INSERTDRV:
			n = va_arg(args, int);
			devp = va_arg(args, devinfo *);
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, devp, sizeof(*devp));
			sendstring(fd, devp -> name);
			break;
#endif
		case TE_SAVETTYIO:
			n = va_arg(args, int);
			val = (n >= 0 && duptty[n]) ? TIO_BUFSIZ : 0;
			sendbuf(fd, &n, sizeof(n));
			sendbuf(fd, &val, sizeof(val));
			if (n >= 0 && duptty[n]) sendbuf(fd, duptty[n], val);
			break;
#ifdef	DEP_IME
		case TE_ADDROMAN:
			cp = va_arg(args, char *);
			func1 = va_arg(args, char *);
			sendstring(fd, cp);
			sendstring(fd, func1);
			break;
#endif
		case TE_INTERNAL:
			n = va_arg(args, int);
			cp = va_arg(args, char *);
			sendbuf(fd, &n, sizeof(n));
			sendstring(fd, cp);
			break;
		default:
			break;
	}
	va_end(args);
	syncptyout(fd, TE_UNLOCKFRONT);
}

static VOID NEAR awakechild(command, arg, flags)
CONST char *command, *arg;
int flags;
{
	if (isttyiomode) {
		flags |= F_TTYIOMODE;
		if (isttyiomode > 1) flags |= F_TTYNL;
	}

	sendword(emufd, TE_AWAKECHILD);
	sendword(emufd, win);
	sendbuf(emufd, &flags, sizeof(flags));
	sendstring(emufd, command);
	sendstring(emufd, arg);
	sendstring(emufd, fullpath);
}

#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
static int trap_hup(VOID_A)
{
	int i;

	for (i = 0; i < maxjobs; i++) {
		if (!(joblist[i].pids)) continue;
		Xkillpg(joblist[i].pids[0], SIGHUP);
# ifdef	SIGCONT
		Xkillpg(joblist[i].pids[0], SIGCONT);
# endif
	}

	VOID_C signal2(SIGHUP, SIG_DFL);
	VOID_C kill(mypid, SIGHUP);
	_exit(SIGHUP + 128);

	return(0);
}

static int NEAR recvmacro(commandp, argp, flagsp)
char **commandp, **argp;
int *flagsp;
{
	char *command, *arg, *cwd;
	int flags;

	Xttyiomode(1);
	if (recvbuf(ttyio, &flags, sizeof(flags)) < 0
	|| recvstring(ttyio, &command) < 0)
		return(-1);
	if (recvstring(ttyio, &arg) < 0) {
		Xfree(command);
		return(-1);
	}
	if (recvstring(ttyio, &cwd) >= 0 && cwd) {
		VOID_C chdir2(cwd);
		Xfree(cwd);
	}

	keyflush();
	if (!(flags & F_TTYIOMODE)) Xstdiomode();
	else if (!(flags & F_TTYNL)) Xttyiomode(0);

	Xfree(*commandp);
	Xfree(*argp);
	*commandp = command;
	*argp = arg;
	*flagsp = flags;

	return(0);
}
#endif	/* DEP_ORIGSHELL && !NOJOB */

static int NEAR callmacro(command, arg, flags)
CONST char *command, *arg;
int flags;
{
	int i, n;

	if (parentfd < 0) {
		changewin(MAXWINDOWS, (p_id_t)-1);
		if (ptymode) {
			hideclock = 2;
			warning(0, NOPTY_K);
		}
		killpty(win, NULL);
		for (i = 0; i < MAXWINDOWS; i++) if (ptylist[i].pid) break;
		if (i < MAXWINDOWS) {
			hideclock = 2;
			if (!yesno(KILL_K)) return(-1);
		}
		killallpty();
	}

	if (flags & F_DOSYSTEM) n = dosystem(command);
#ifndef	DEP_ORIGSHELL
	else if (flags & F_EVALMACRO) n = execusercomm(command, arg, flags);
#endif
	else n = execmacro(command, arg, flags);

	return(n);
}

int ptymacro(command, arg, flags)
CONST char *command, *arg;
int flags;
{
#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
	char *command2, *arg2;
#endif
	p_id_t pid;
	char *tty, *ws, path[MAXPATHLEN], buf[TIO_BUFSIZ + TIO_WINSIZ];
	u_char uc;
	int i, n, fd, fds[2];

#ifdef	DEP_ORIGSHELL
	if (isshptymode()) n = (flags & F_DOSYSTEM) ? 0 : -1;
	else
#endif
	n = (ptymode && !ptyinternal && ttyio >= 0) ? 0 : -1;
	if (n < 0) return(callmacro(command, arg, flags));

	if (ptylist[win].pid && emufd >= 0) {
		awakechild(command, arg, flags);
		return(frontend());
	}

	savetermio(ttyio, &tty, &ws);
	if (tty) {
		memcpy(buf, tty, TIO_BUFSIZ);
		Xfree(tty);
		tty = buf;
	}
	if (ws) {
		memcpy(&(buf[TIO_BUFSIZ]), ws, TIO_WINSIZ);
		Xfree(ws);
		ws = &(buf[TIO_BUFSIZ]);
	}

	if (genbackend() < 0 || pipe(fds) < 0)
		return(callmacro(command, arg, flags));
	VOID_C fcntl(fds[0], F_SETFL, O_NONBLOCK);
	VOID_C fcntl(fds[1], F_SETFL, O_NONBLOCK);

	if (ptytmpfile) fd = -1;
	else if ((fd = mktmpfile(path)) >= 0) {
		ptytmpfile = Xstrdup(path);
		VOID_C Xclose(fd);
	}

#ifdef	DEP_KCONV
	changeinkcode();
	changeoutkcode();
#endif

	n = sigvecset(0);
	if ((pid = Xfork()) < (p_id_t)0) {
		if (fd >= 0 && ptytmpfile) {
			rmtmpfile(ptytmpfile);
			Xfree(ptytmpfile);
			ptytmpfile = NULL;
		}
		sigvecset(n);
		safeclose(fds[0]);
		safeclose(fds[1]);
		return(callmacro(command, arg, flags));
	}

	if (!pid) {
#ifdef	DEP_ORIGSHELL
		mypid = getpid();
#endif
#ifdef	CYGWIN
		closeallpty();
#endif
		if (Xlogin_tty(ptylist[win].path, tty, ws) < 0) _exit(1);
#ifdef	DEP_ORIGSHELL
		if (isshptymode()) /*EMPTY*/;
		else
#endif
		n_line = FILEPERROW;
		setwsize(STDIN_FILENO, n_column, n_line);

		for (i = 0; i < MAXWINDOWS; i++) {
			ptylist[i].pid = (p_id_t)0;
			Xfree(ptylist[i].path);
			ptylist[i].path = NULL;
			ptylist[i].pipe = -1;
			ptylist[i].status = 0;
		}
		emupid = (p_id_t)0;
		safeclose(emufd);
		emufd = -1;
		if (Xfileno(ttyout) == ttyio) Xdup2(STDIN_FILENO, ttyio);
		else {
			fd = Xfileno(ttyout);
			closetty(&fd, &ttyout);
			opentty(&fd, &ttyout);
			Xdup2(fd, ttyio);
			safeclose(fd);
		}

		setdefterment();
		setdefkeyseq();

		safeclose(fds[0]);
		parentfd = newdup(fds[1]);
		closeonexec(parentfd);
		uc = '\n';
		sendbuf(parentfd, &uc, sizeof(uc));

#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
		command2 = arg2 = NULL;
#endif
		for (;;) {
			syncptyout(-1, -1);
			setlinecol();
			n = evalstatus(callmacro(command, arg, flags));
#if	defined (DEP_ORIGSHELL) && !defined (NOJOB)
			for (i = 0; i < maxjobs; i++)
				if (joblist[i].pids) break;
			if (i >= maxjobs) break;
			VOID_C signal2(SIGHUP, (sigcst_t)trap_hup);
			sendparent(TE_CHANGESTATUS, n);
			if (recvmacro(&command2, &arg2, &flags) < 0) break;
			command = command2;
			arg = arg2;
#else	/* !DEP_ORIGSHELL || NOJOB */
			break;
#endif	/* !DEP_ORIGSHELL || NOJOB */
		}

		safeclose(parentfd);

		_exit(n);
	}

	safeclose(fds[1]);
	ptylist[win].pid = pid;
	ptylist[win].pipe = newdup(fds[0]);
	ptylist[win].status = -1;

	sigvecset(n);
	if (recvbuf(ptylist[win].pipe, &uc, sizeof(uc)) < 0)
		return(callmacro(command, arg, flags));

	return(frontend());
}

VOID killpty(n, statusp)
int n, *statusp;
{
	if (ptylist[n].pid) {
		VOID_C Xkillpg(ptylist[n].pid, SIGHUP);
#ifdef	SIGCONT
		VOID_C Xkillpg(ptylist[n].pid, SIGCONT);
#endif
		VOID_C waitstatus(ptylist[n].pid, 0, statusp);
		ptylist[n].pid = (p_id_t)0;
		ptylist[n].status = 0;
	}
	changewin(n, (p_id_t)0);
	changewin(MAXWINDOWS, (p_id_t)-1);

	safeclose(ptylist[n].pipe);
	ptylist[n].pipe = -1;
}

VOID killallpty(VOID_A)
{
	int i;

	for (i = 0; i < MAXWINDOWS; i++) {
		killpty(i, NULL);
		Xfree(ptylist[i].path);
		ptylist[i].path = NULL;
	}

#ifdef	CYGWIN
	closeallpty();
#endif
	if (emupid) {
		VOID_C kill(emupid, SIGHUP);
#ifdef	SIGCONT
		VOID_C kill(emupid, SIGCONT);
#endif
		VOID_C waitstatus(emupid, 0, NULL);
		emupid = (p_id_t)0;
	}
	if (ptytmpfile) {
		rmtmpfile(ptytmpfile);
		Xfree(ptytmpfile);
		ptytmpfile = NULL;
	}

	safeclose(emufd);
	emufd = -1;
}

int checkpty(n)
int n;
{
	int status;

	if (!(ptylist[n].pid)) return(0);
	if (waitstatus(ptylist[n].pid, WNOHANG, &status) < 0) return(0);
	ptylist[n].pid = (p_id_t)0;
	ptylist[n].status = status;
	killpty(n, &status);

	return(-1);
}

int checkallpty(VOID_A)
{
	int i, n;

	n = 0;
	for (i = 0; i < MAXWINDOWS; i++) if (checkpty(i) < 0) n = -1;

	return(n);
}
#endif	/* DEP_PTY */
