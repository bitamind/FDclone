/*
 *	mkfuncno.c
 *
 *	preprocesser for "funcno.h"
 */

#define	__FD_PRIMAL__
#include "fd.h"
#include "types.h"
#ifdef	_NOCATALOG
#define	_TBL_(fn, id, hl, fl)	{NULL, id, NULL, 0}
#else
#define	_TBL_(fn, id, hl, fl)	{NULL, id, 0, 0}
#endif
#include "functabl.h"

int main __P_((int, char *CONST []));


/*ARGSUSED*/
int main(argc, argv)
int argc;
char *CONST argv[];
{
	FILE *fp;
	int i, len;

	if (!strcmp(argv[1], "-")) fp = stdout;
	else if (!(fp = fopen(argv[1], "w"))) {
		VOID_C fprintf(stderr, "%s: Cannot open.\n", argv[1]);
		return(1);
	}

	VOID_C fprintf(fp, "/*\n");
	VOID_C fprintf(fp, " *\t%s\n", (fp != stdout) ? argv[1] : "STDOUT");
	VOID_C fprintf(fp, " *\n");
	VOID_C fprintf(fp, " *\tfunction No. table\n");
	VOID_C fprintf(fp, " */\n");
	VOID_C fprintf(fp, "\n");

	for (i = 0; i < (int)sizeof(funclist) / sizeof(functable); i++) {
		VOID_C fprintf(fp, "#define\t%s\t", funclist[i].ident);
		len = strlen(funclist[i].ident);
		while ((len += 8) < 16) VOID_C fputc('\t', fp);
		VOID_C fprintf(fp, "\t%d\n", i);
	}
	VOID_C fprintf(fp, "\n#define\tFUNCLISTSIZ\t\t%d\n", i);

	if (fp != stdout) VOID_C fclose(fp);

	return(0);
}
