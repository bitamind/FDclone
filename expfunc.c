/*
 *	expfunc.c
 *
 *	function expander for the obsolete /bin/sh
 */

#include "headers.h"
#include <ctype.h>

#define	MAXLINEBUF		255
#define	MAXFUNCNO		32
#define	MAXARGS			10

extern VOID exit __P_((int));

static char *NEAR skipspace __P_((char *));
static char *NEAR Xstrlcat __P_((char *, CONST char *, int));
static char *NEAR Xstrndup __P_((CONST char *, int));
static char *NEAR isfunction __P_((char *));
static int NEAR entryfunc __P_((char *));
static char *NEAR checkhere __P_((char *));
static int NEAR getargs __P_((char **, char *[]));
static char *NEAR expargs __P_((CONST char *, char *[]));
static int NEAR searchfunc __P_((CONST char *));
static int NEAR evalline __P_((char *, char *[]));
static char *NEAR getline __P_((FILE *));
int main __P_((int, char *CONST []));

static int funcno = 0;
static char *func[MAXFUNCNO];
static int funclen[MAXFUNCNO];
static char *funcbody[MAXFUNCNO];


static char *NEAR skipspace(s)
char *s;
{
	while (*s == ' ' || *s == '\t') s++;

	return(s);
}

static char *NEAR Xstrlcat(s1, s2, n)
char *s1;
CONST char *s2;
int n;
{
	char *s;
	int i;

	i = strlen(s1);
	n -= i;
	s = &(s1[i]);

	for (i = 0; i < n && s2[i]; i++) s[i] = s2[i];
	s[i] = '\0';

	return(s1);
}

static char *NEAR Xstrndup(s, n)
CONST char *s;
int n;
{
	char *tmp;

	if (!s) return(NULL);
	if (!(tmp = (char *)malloc(n + 1))) exit(1);
	memcpy(tmp, s, n);
	tmp[n] = '\0';

	return(tmp);
}

static char *NEAR isfunction(line)
char *line;
{
	char *cp;
	int len;

	if (funcno >= MAXFUNCNO) return(NULL);

	line = skipspace(line);

	for (len = 0; line[len]; len++)
		if (line[len] != '_' && !isalpha((int)(line[len]))) break;
	if (!len) return(NULL);

	cp = skipspace(&(line[len]));
	if (*cp != '(') return(NULL);

	cp = skipspace(&(cp[1]));
	if (*cp != ')') return(NULL);

	cp = skipspace(&(cp[1]));
	if (*cp != '{') return(NULL);

	func[funcno] = Xstrndup(line, len);
	funclen[funcno] = len;
	funcbody[funcno] = NULL;

	return(&(cp[1]));
}

static int NEAR entryfunc(line)
char *line;
{
	char *cp;
	int i, len, quote;

	i = 1;
	quote = '\0';
	if (*line == '}') {
		i = 0;
		line = "";
	}
	else for (cp = line; *cp; cp++) {
		if (*cp == quote) quote = '\0';
		else if (quote == '\'') continue;
		else if (*cp == '\\' && cp[1]) cp++;
		else if (quote) continue;
		else if (*cp == '\'' || *cp == '"') quote = *cp;
		else if (*cp == '}') {
			*cp = '\0';
			i = 0;
		}
	}

	len = strlen(line);
	if (!i) len += 3;
	if (!funcbody[funcno]) {
		len += 2;
		funcbody[funcno] = (char *)malloc(len + 1);
		if (!funcbody[funcno]) exit(1);
		strcpy(funcbody[funcno], "{ ");
	}
	else {
		len += strlen(funcbody[funcno]);
		if (*line) len++;
		funcbody[funcno] = (char *)realloc(funcbody[funcno], len + 1);
		if (!funcbody[funcno]) exit(1);
		if (*line) VOID_C Xstrlcat(funcbody[funcno], "\n", len);
	}

	VOID_C Xstrlcat(funcbody[funcno], line, len);
	if (!i) VOID_C Xstrlcat(funcbody[funcno++], "; }", len);

	return(i);
}

static char *NEAR checkhere(line)
char *line;
{
	char *cp;
	int len, quote;

	quote = '\0';
	for (cp = line; *cp; cp++) {
		if (*cp == quote) quote = '\0';
		else if (quote == '\'') continue;
		else if (*cp == '\\' && cp[1]) cp++;
		else if (quote) continue;
		else if (*cp == '\'' || *cp == '"') quote = *cp;
		else if (*cp == '<' && cp[1] == '<') break;
	}
	if (*cp != '<') return(NULL);

	cp = skipspace(&(cp[2]));
	for (len = 0; cp[len]; len++)
		if (cp[len] == ' ' || cp[len] == '\t') break;

	return(Xstrndup(cp, len));
}

static int NEAR getargs(linep, args)
char **linep, *args[];
{
	char *cp, *new, *line, *buf;
	int i, n, len, quote, size;

	for (i = 0; i < MAXARGS; i++) args[i] = NULL;

	line = *linep;
	for (i = 0; i < MAXARGS; i++) {
		new = buf = NULL;
		n = size = 0;
		cp = line = skipspace(line);
		quote = '\0';
		for (;;) {
			if (!*cp) {
				if (!quote) break;

				len = strlen(line);
				if (!buf) buf = (char *)malloc(len + 1);
				else buf = (char *)realloc(buf,
					size + 1 + len + 1);
				if (!buf) exit(1);

				if (size) {
					buf[size++] = '\n';
					n++;
				}
				memcpy(&(buf[size]), line, len + 1);
				size += len;

				cp = line = getline(stdin);
				if (!cp) break;
				continue;
			}

			if (*cp == quote) quote = '\0';
			else if (quote == '\'') /*EMPTY*/;
			else if (*cp == '\\') {
				if (cp[1] == '\n') break;
				else if (cp[1]) {
					cp++;
					n++;
				}
				else {
					new = getline(stdin);
					if (!new || line > cp) break;
					cp = line = new;
					new = NULL;
					continue;
				}
			}
			else if (quote) /*EMPTY*/;
			else if (*cp == '\'' || *cp == '"') quote = *cp;
			else if (strchr(" \t\n!&);<>`|}", *cp)) break;

			cp++;
			n++;
		}

		if (n) {
			if (buf) {
				len = strlen(line);
				buf = (char *)realloc(buf, size + 1 + len + 1);
				if (!buf) exit(1);
				buf[size++] = '\n';
				memcpy(&(buf[size]), line, len + 1);
				size += len;
				line = buf;
			}

			if (*line == '\'' || *line == '"') {
				n--;
				if (*(cp - 1) == *line) n--;
				line++;
			}
			args[i] = Xstrndup(line, n);
		}
		if (buf) free(buf);

		if (new) line = new;
		else if (*cp == '\\' && cp[1] == '\n') line = &(cp[2]);
		else if (*cp == ' ' || *cp == '\t') line = cp;
		else {
			i++;
			break;
		}
	}
	*linep = cp;

	return(i);
}

static char *NEAR expargs(line, args)
CONST char *line;
char *args[];
{
	char *cp, *buf, *tmp, *top;
	int i, len, alen, vlen, rlen, quote;

	len = strlen(line);
	buf = Xstrndup(line, len);

	quote = '\0';
	cp = buf;
	while (*cp) {
		if (*cp == quote) quote = '\0';
		else if (quote == '\'') /*EMPTY*/;
		else if (*cp == '\\' && cp[1]) cp++;
		else if (*cp == '$') {
			top = cp;
			if (*(++cp) == '{') cp++;
			if (*cp >= '1' && *cp <= '9') {
				i = *cp - '1';
				if (*(++cp) == '}') cp++;

				alen = cp - top;
				rlen = len - (cp - buf);
				vlen = (args[i]) ? strlen(args[i]) : 0;
				len = len - alen + vlen;

				if (vlen > alen) {
					tmp = (char *)realloc(buf, len + 1);
					if (!tmp) exit(1);
					cp = &(tmp[cp - buf]);
					top = &(tmp[top - buf]);
					buf = tmp;
				}

				memmove(&(top[vlen]), cp, rlen + 1);
				if (vlen) memcpy(top, args[i], vlen);
				continue;
			}
		}
		else if (quote) /*EMPTY*/;
		else if (*cp == '\'' || *cp == '"') quote = *cp;

		cp++;
	}

	return(buf);
}

static int NEAR searchfunc(s)
CONST char *s;
{
	int i, len;

	for (i = 0; i < funcno; i++) {
		len = funclen[i];
		if (!strncmp(s, func[i], len))
			if (s[len] == ' ' || s[len] == '\t') return(i);
	}

	return(-1);
}

static int NEAR evalline(line, args)
char *line, *args[];
{
	char *cp, *newargs[MAXARGS];
	int i, quote;

	if (args) line = expargs(line, args);

	quote = '\0';
	for (cp = line; *cp; cp++) {
		if (*cp == quote) quote = '\0';
		else if (quote) /*EMPTY*/;
		else if (*cp == '\'' || *cp == '"') quote = *cp;
		else if ((i = searchfunc(cp)) >= 0) {
			cp += funclen[i];
			getargs(&cp, newargs);
			evalline(funcbody[i], newargs);
			for (i = 0; i < MAXARGS; i++)
				if (newargs[i]) free(newargs[i]);
			cp--;
			continue;
		}

		VOID_C fputc(*cp, stdout);
	}

	i = strlen(line);
	if (args) free(line);
	else VOID_C fputc('\n', stdout);

	return(i);
}

static char *NEAR getline(fp)
FILE *fp;
{
	static char buf[MAXLINEBUF + 1];
	int len;

	for (;;) {
		*buf = '\0';
		if (!fgets(buf, MAXLINEBUF, fp)) return(NULL);
		len = strlen(buf);
		if (len > 0 && buf[--len] == '\n') buf[len] = '\0';
		if (*(skipspace(buf)) != '#') break;
		VOID_C fprintf(stdout, "%s\n", buf);
	}

	return(buf);
}

/*ARGSUSED*/
int main(argc, argv)
int argc;
char *CONST argv[];
{
	char *cp, *heredoc, *line;
	int infunc;

	infunc = 0;
	heredoc = NULL;
	func[0] = "return";
	funclen[0] = strlen(func[0]);
	funcbody[0] = "(exit $1)";
	funcno = 1;

	while ((line = getline(stdin))) {
		if (heredoc) {
			VOID_C fprintf(stdout, "%s\n", line);
			if (!strcmp(line, heredoc)) {
				free(heredoc);
				heredoc = NULL;
			}
		}
		else {
			heredoc = checkhere(line);
			if (infunc) infunc = entryfunc(line);
			else if ((cp = isfunction(line)))
				infunc = (*cp) ? entryfunc(cp) : 1;
			else evalline(line, NULL);
		}
	}

	return(0);
}
