/*
 * GTools C compiler
 * =================
 * source file :
 * main entry point
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define CMAIN_C
#define DECLARE

#ifdef PC
#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#define ENVPATH_SEPARATOR ';'
#else
#ifdef UNIXOID
#define DIR_SEPARATOR '/'
#define ENVPATH_SEPARATOR ':'
#else
#error Please define a directory and a $PATH separator for your platform.
#endif
#endif
#endif

#ifdef PC
/*
#define memcmp memcmp2
#define strcmp stricmp
//#define strlen strlen2
#define abs abs2
#define _rotl _rotl2
#define _rotr _rotr2
//#include	<signal.h>

// --> what was this crap ???
*/
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <assert.h>
#endif

#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

extern char *curname;

#ifdef SPEED_OPT
int default_speed_opt_value CGLOB;
#endif

static void openfile();
static void closefile();
static void initialize_files();
static void close_files();
#ifdef LISTING
static			summary();
#endif

#ifdef PC
void fatal(char *s);
#else
void fatal(char *s) __attribute__ ((noreturn));
#endif

#ifdef PC
#ifndef debug
void debug(int c) {
	fprintf(stderr,"debug step '%c'\n",c);
}
#endif
#ifdef EXCEPTION_MGT
static void exception(int sig) {
	msg2("\n\nSIGNAL %d -- TERMINATING.\n", sig);
	fatal("EXCEPTION");
}
#endif
#define _exit __exit
int pause_on_error=0;
void _exit(int code) {
	clean_up();
	if (pause_on_error)
		getchar();
	exit(code);
}
#endif

char export_pfx[20]; short_size export_pfx_len;

#ifdef TWIN_COMPILE
int twinc_necessary CGLOB;
int *twinc_info CGLOB,*twinc_info0 CGLOB,*twinc_prev CGLOB,*twinc_prev0 CGLOB;
#endif

#ifdef PC
	char *calcfolder=0;
	static char _calcfolder[100];
	char *outputname=0,*calcname=0;
	static char _calcname[100];
	
	#include <ctype.h>
	#include <time.h>
	#if 0
	void TempName(char *dest) {
		int i;
		srand(clock());
		*dest++='~';
		*dest++='g';
		*dest++='t';
		*dest++='c';
		for (i=0;i<4;i++)
			*dest++='0'+(rand()*10)/(RAND_MAX+1);
		strcpy(dest,".tmp");
	}
	#endif

	/* warning: duplicated in getsym.c! */
	enum OptionModes { optn_parse=0, optn_global_init, optn_global_init_post, optn_preproc_init };
	size_t option_parse(size_t listc,char **listv,enum(OptionModes) ex_mode);
	void option_parse_all(enum(OptionModes) ex_mode);

	char *incfolders[10] CGLOBL;
	size_t incfoldernum CGLOB;

	size_t optnc,optnmax; char **optnv;
	size_t filec,filemax; char **filev;

#define LST_INIT(lst,len) (lst##c=0, lst##max=(len), lst##v=alloca((len)*sizeof(*lst##v)))
#define LST_ADD(lst,val) (lst##c>=lst##max?exit(EXIT_FAILURE):(void)0, lst##v[lst##c++]=(val))

	void cmdline_parse(size_t argc, char **argv) {
		size_t i;
		int force_files = 0;
		for (i=1; i<argc; ) {
			size_t listc = argc-i, skip;
			char **listv = &argv[i];
			char *arg = listv[0];
			if (!force_files && arg[0] == '-') {
				if (!strcmp(arg,"--")) {
					force_files = 1;
					i++;
				} else {
					if ((skip = option_parse(listc,listv,optn_parse))) {
						size_t j;
						for (j=0; j<skip; j++)
							LST_ADD(optn,listv[j]);
						i+=j;
					} else {
						msg2("Invalid option '%s', ignoring option.\n", arg);
						i++;
					}
				}
			} else {
				LST_ADD(file,arg);
				i++;
			}
		}
	}

	void search_installdir(char *exe_name) {
		char *gtcpath=alloca(strlen(exe_name)+40);
		strcpy(gtcpath,exe_name);
		if (strrchr(gtcpath,DIR_SEPARATOR)) strrchr(gtcpath,DIR_SEPARATOR)[1]=0;	// strip 'gtc.exe'
		else {
	#if 0
			gtcpath="";	// 'gtc.exe' is in the current folder... (this assumption is OK under Win9x)
	#else
			// things don't stop here as WinXP sets argv[0] to be 'gtc.exe' if it is in the
			// PATH environment variable... (and the same goes for Unixoids)
			char *path=getenv("PATH");
			while ((gtcpath="",path)) {
				char *limit=strchr(path,ENVPATH_SEPARATOR),*base_limit=limit; char *buf;
				FILE *fp;
				if (!limit) limit=path+strlen(path);
				if (path[0]=='"' && path[limit-path-1]=='"')
					path++,limit--;
				gtcpath=alloca(limit-path+2);
				memcpy(gtcpath,path,(size_t)(limit-path)); gtcpath[limit-path]=DIR_SEPARATOR; gtcpath[limit-path+1]=0;
				buf=alloca(strlen(gtcpath)+sizeof("gtc.exe"));
				strcpy(buf,gtcpath);
				strcat(buf,"gtc.exe");
				if ((fp=fopen(buf,"rb"))) {
					fclose(fp);
					break;
				}
		#ifndef _WIN32
				strcpy(buf,gtcpath);
				strcat(buf,"gtc");
				if ((fp=fopen(buf,"rb"))) {
					fclose(fp);
					break;
				}
		#endif
				path = base_limit ? base_limit+1 : NULL;
			}
		}
	#endif
		{
		char *incl_cmd=malloc(sizeof("-I")-1+strlen(gtcpath)+sizeof("include"));
		assert(incl_cmd!=0);
		strcpy(incl_cmd,"-I");
		strcat(incl_cmd,gtcpath);
		strcat(incl_cmd,"include");
		LST_ADD(optn,incl_cmd);
		}
	}

	int main(int _argc, char **argv) {
		size_t argc = (size_t)_argc;
		LST_INIT(optn,argc+10);
		LST_INIT(file,argc+10);
	#ifdef UNIXOID
		pause_on_error=0;
	#endif

		// initialize optn* and file*
		cmdline_parse(argc, argv);
		search_installdir(argv[0]);
	#ifdef INCL_PATH
		LST_ADD(optn,"-I"INCL_PATH);
	#endif

		// now setup options
		//	defaults
		calcfolder=NULL; outputname=NULL; calcname=NULL;
		tp_econst = tp_int;
		tp_econst.val_flag=1;
		//	user-defined options
		option_parse_all(optn_global_init);
		option_parse_all(optn_global_init_post);

		if (verbosity&VERBOSITY_PRINT_SEARCH_DIRS)
			verbose_print_searchdirs();

	#ifdef TWIN_COMPILE
		twinc_info0=0;
		do {
			twinc_prev0=twinc_prev=twinc_info0;
			twinc_info=twinc_info0=malloc(100000);
			twinc_necessary=0;
	#endif
			initialize_files();
			out_init();
/*			initsym();
			getch();
			getsym();
			compile();*/
			initpch();
			{
				size_t optnc_save = optnc; // for twin compilation
				size_t i;
				for (i=0; i<filec; i++) {
					if (i==1)	// the current version of the library needs it :(
						LST_ADD(optn,"-D__SECONDARY_FILE__");
					openfile(i);
					do_compile();
					closefile();
				}
				optnc = optnc_save;
			}
			closepch();
		#ifdef LISTING
			if (list_option)
				summary();
			else
				/* This emits all the global and external directives */
				list_table(&gsyms, 0);
		#endif
		#ifdef VERBOSE
			msg("\n -- %d errors found.\n", total_errors);
		#endif
			out_close();
	//		rel_global();	in out_close()
			close_files();
	#ifdef TWIN_COMPILE
			if (twinc_prev0) free(twinc_prev0);
		} while (twinc_necessary);
		free(twinc_info0);
	#endif
	#ifdef VERBOSE
		decl_time -= parse_time;
		parse_time -= gen_time + flush_time;
		gen_time -= opt_time;
		msg("Times: %6ld + %6ld + %6ld + %6ld + %6ld\n",
				decl_time, parse_time, opt_time, gen_time, flush_time);
	#endif
		if (total_errors > 0)
			_exit(1);
		if (temp_mem)
			printf("TEMP MEM LEAK\n"), temp_mem=0;
		return 0;
	}
#else /* !defined(PC) */
#ifdef OPTIMIZE_BSS
	#define PRI_MAIN
		void _zmain(void);
		#include "identity.h"
		int has_error CGLOB;
	#ifndef GTDEV
		void _main(void) {
			bssdata=malloc(BSS_SIZE);
			if (!bssdata) return;
			memset(bssdata,0,BSS_SIZE);
			has_error=1;
			asm("movem.l d0-d7/a0-a4/a6,-(a7)");	/* do not include 'a5' :o */
			_zmain();
			asm("movem.l (a7)+,d0-d7/a0-a4/a6");
			bssdata=identity(bssdata);	/* volatilize 'bssdata' =) */
			if (bssdata) {	/* in case 'input' or 'output' couldn't be opened... */
				if (bin) free(bin);
				fclose(output);
				fclose(input);
				free(bssdata);
			}
		}
	#else
		void _main(void) {
			ST_helpMsg("Please use the IDE to compile.");
			ngetchx();
		}
	#endif
	#ifdef GTDEV
		void _gtdevmain(void) {
			has_error=1;
			asm("movem.l d0-d7/a0-a4/a6,-(a7)");	/* do not include 'a5' :o */
			_zmain();
			asm("movem.l (a7)+,d0-d7/a0-a4/a6");
			bssdata=identity(bssdata);	/* volatilize 'bssdata' =) */
			if (bssdata) {	/* in case 'input' or 'output' couldn't be opened... */
				if (bin) free(bin);
				fclose(output);
				fclose(input);
			}
		}
	#endif
	void _zmain(void) {
#else
	void _main(void) {
#endif
		int n;
	#ifndef GTDEV
		clrscr();
	#endif
//		__HALT;
/*		tp_econst = tp_int;
		tp_econst.val_flag=1;*/
	#ifndef PRI_MAIN
	#ifdef OPTIMIZE_BSS
		bssdata=malloc(BSS_SIZE);
		if (!bssdata) return;
		memset(bssdata,0,BSS_SIZE);
	#endif
	#endif

	#ifndef GTDEV
	#define in_file "in"
	#ifdef AS
	#define out_file "outbin"
	#else
	#define out_file "out"
	#endif
	#endif
		if (!(input=fopen(in_file,"r"))) goto quit;
	#ifdef AS
		if (!(output=fopen(out_file,"wb"))) { fclose(input); quit: free(bssdata); bssdata=0; return; }
	#else
		if (!(output=fopen(out_file,"w"))) { fclose(input); quit: free(bssdata); bssdata=0; return; }
	#endif

		/*********************************************************
		 * [!BUG-SOURCE!] THE FOLLOWING LINE IS FAIRLY GORE SO   *
		 *  *ANY* MODIFICATION WHATSOEVER TO _MAIN (OR EVEN ANY  *
		 *  COMPILER UPDATE...) MIGHT BREAK COMPATIBILITY!       *
		 *                                                       */
	#ifndef GTDEV
		/**/  asm("lea 12(%sp),%a0\n	move.l %a0,exit_a7");  /**/
	#else
		/**/  asm("lea  4(%sp),%a0\n	move.l %a0,exit_a7");  /**/
	#endif
		/*                                                       *
		 *********************************************************/
		out_init();
		initpch();
		strcpy(export_pfx,strrchr(in_file,'\\')?:in_file);
		strcat(export_pfx,"__"); export_pfx_len=strlen(export_pfx);
		do_compile();
		closepch();
		debug('E');
		out_close();
		debug('Q');
//		rel_global();	in out_close()
		if (ferror(output)) fatal("Memory");
	#ifndef PRI_MAIN
		fclose(output);
		fclose(input);
	#endif
	#ifndef GTDEV
		if (temp_mem) printf("TEMP MEM LEAK\n"), temp_mem=0;
	#endif
/*		if (total_errors > 0) { printf("\nThere were errors."); while (!kbhit()); }
		else*/ {
	#ifdef PRI_MAIN
			has_error=0;
	#endif
	#ifndef GTDEV
			FILE *fp=fopen("gtcerr","wb");
			if (fp) {
				fputc(0,fp);
				fputc(POSINT_TAG,fp);
				fclose(fp);
			}
			printf("\nsuccess");
			n=4000;
			while (n-- && !kbhit());
			if (kbhit()) ngetchx();
	#endif
		}
	#ifndef PRI_MAIN
	#ifdef OPTIMIZE_BSS
		free(bssdata);
	#endif
	#endif
	}
#endif /* !defined(PC) */


#ifdef PC
int forbid_bss=0;

char *fill_calcvar(char *buffer, char *input) {
	int i=0;
	while (*input) {
		char c=tolower(*input++);
		if (c=='-' || c==' ' || c=='.')
			c='_';
		if (isalnum(c) && i<8)
			buffer[i++] = c;
	}
	buffer[i]=0;
	if (!buffer[0])
		fatal("bad calculator variable name");
	return buffer;
}

// Takes as an input the list of remaining arguments, returning:
// * 0: can't parse the option
// * n>0: the option was parsed successfully, n = number of arguments
//		to skip for this option
size_t option_parse(size_t listc,char **listv,enum(OptionModes) ex_mode) {
	char *s = listv[0];
	size_t value = 1; // number of arguments read so far
	char *next_chunk, *_next_chunk;
	#define strscmp(s,t) (next_chunk=_next_chunk="", strcmp(s,t))
	#define strbcmp(s,t) (next_chunk=_next_chunk=s+strlen(t), strncmp(s,t,strlen(t)))
	// get_next_chunk is only necessary if you want to handle options split into several arguments
	#define get_next_chunk() \
			do { \
				next_chunk = *_next_chunk ? _next_chunk : listc > value ? listv[value++] : NULL; \
				if (!next_chunk) return 0; \
			} while (0)

	// PURELY SYNTACTIC TRANSFORMATIONS
	if (!strscmp(s,"-exe"))
		s = "-DEXE_OUT";

	// STANDALONE OPTIONS (strscmp)
	if (!strscmp(s,"-o")) {
		get_next_chunk();
		if (ex_mode==optn_global_init)
			outputname = next_chunk;
	} else if (!strscmp(s,"-v")) {
#ifdef PC
		verbose=1;
#endif
#ifdef PC
	} else if (!strscmp(s,"-print-search-dirs")) {
		verbosity|=VERBOSITY_PRINT_SEARCH_DIRS;
	} else if (!strscmp(s,"-print-includes")) {
		verbosity|=VERBOSITY_PRINT_INCLUDES;
#endif
	} else if (!strscmp(s,"-mno-bss")) {
		if (ex_mode==optn_global_init)
			forbid_bss=1;
	} else if (!strcmp(s,"-O2")) {
		default_speed_opt_value = 1;
	} else if (!strcmp(s,"-O3")) {
		default_speed_opt_value = 3;
	} else if (!strcmp(s,"-Os")) {
		default_speed_opt_value = -1;
	// CHUNKED OPTIONS (strbcmp)
	} else if (!strbcmp(s,"-I")) {
		get_next_chunk();
		if (ex_mode==optn_global_init)
			if (incfoldernum<sizeof(incfolders)/sizeof(*incfolders))
				incfolders[incfoldernum++] = next_chunk;
	} else if (!strbcmp(s,"--std-include=")) {
		get_next_chunk();
		if (ex_mode==optn_global_init_post)
			if (incfoldernum<sizeof(incfolders)/sizeof(*incfolders))
				incfolders[incfoldernum++] = next_chunk;
	} else if (!strbcmp(s,"-D")) {
		if (ex_mode==optn_preproc_init) {
			struct sym *sp = (SYM *)xalloc((int)sizeof(SYM), _SYM);
			char *name = next_chunk;
			char *name_end = strchr(name,'=');
			if (!name_end)
				name_end = name+strlen(name);
			sp->name = (char *)xalloc(name_end-name+1, STR);
			memcpy(sp->name, name, (size_t)(name_end-name));
			sp->name[name_end-name] = 0;
			sp->value.s = *name_end ? name_end+1 : "1";
			insert(sp,&defsyms);
		}
	// LONG OPTIONS
	} else if (!strbcmp(s,"--")) {
		// STANDALONE OPTIONS (strscmp)
		if (!strscmp(s,"--nobss")) {
			if (ex_mode==optn_global_init)
				forbid_bss=1;
		// CHUNKED OPTIONS (strbcmp)
		} else if (!strbcmp(s,"--folder=")) {
			if (ex_mode==optn_global_init)
				calcfolder = fill_calcvar(_calcfolder,next_chunk);
		} else if (!strbcmp(s,"--output=")) {
			if (ex_mode==optn_global_init)
				calcname = fill_calcvar(_calcname,next_chunk);
		// ERROR
		} else
			return 0;
	// ERROR
	} else
		return 0;
	return value;
	#undef get_next_chunk
	#undef strbcmp
	#undef strscmp
}
void option_parse_all(enum(OptionModes) ex_mode) {
	size_t i;
	for (i=0; i<optnc; )
		i += option_parse(optnc-i, &optnv[i], ex_mode);
}

static char _outputname[300];
char proj_file[300];
#ifdef PC
char tempname[20];
#endif
static void openfile(size_t idx) {
	if (!(input = fopen(curname=filev[idx], "r"))) {
		msg2("can't open input file '%s'\n",curname);
		_exit(2);
	}
}
static void closefile() {
	fclose(input);
}
// obtain the last part delimited by the character c
// (e.g. "xxx/yyy/zzz" -> "zzz", and "zzz" -> "zzz")
static char *strlast(char *s,char c) {
	char *t = strrchr(s,c);
	return t ? t+1 : s;
}

static void initialize_files() {
	if (filec < 1) {
		msg("no input file specified!\n");
		_exit(2);
	}

	if (!outputname) {
		outputname = _outputname;
		strncpy(outputname,filev[0],sizeof(_outputname)-1);
		outputname[sizeof(_outputname)-1]=0;
		if (strrchr(outputname,'.'))
			*strrchr(outputname,'.')=0;
	}
	strncpy(proj_file,outputname,sizeof(proj_file)-4-1);
	proj_file[sizeof(proj_file)-4-1]=0;
	strcat(proj_file,".gtc");

	if (!calcname) {
		char *zcalcname;
		calcname = outputname;
		calcname = strlast(calcname,'\\');	// for Windows
		calcname = strlast(calcname,'/');	// for Unix
		zcalcname = alloca(strlen(calcname)+1);
		strcpy(zcalcname,calcname);
		if (strrchr(zcalcname,'.'))
			*strrchr(zcalcname,'.')=0;
		calcname = fill_calcvar(_calcname,zcalcname);
	}
	strcpy(export_pfx,calcname);
	strcat(export_pfx,"__"); export_pfx_len=strlen(export_pfx);
#ifndef AS
	{
		char *buffer=alloca(strlen(outputname)+strlen(".asm")+1);
		sprintf(buffer,"%s.asm",outputname);
		if (!(output = fopen(buffer, "w"))) {
			msg("can't open output file\n");
			_exit(2);
		}
	}
#endif
#ifdef LISTING
	if (list_option && (list = fopen(LIST_NAME, "w")) == 0) {
		msg("can't open listfile\n");
		_exit(1);
	}
#endif
#ifdef ICODE
	if (icode_option && ((icode = fopen(ICODE_NAME, "w")) == 0)) {
		msg("can't open icode file\n");
		_exit(1);
	}
#endif
}
#endif

#ifdef LISTING
static void summary(void) {
//	if (gsyms.head != NULL) {
		fprintf(list, "\f\n *** global scope symbol table ***\n\n");
		list_table(&gsyms, 0);
//	}
//	if (gtags.head != NULL) {
		fprintf(list, "\n *** structures and unions ***\n\n");
		list_table(&gtags, 0);
//	}
}
#endif

#ifdef PC
static void close_files(void) {
#ifdef LISTING
		if (list_option)
		fclose(list);
#endif
#ifdef ICODE
	if (icode_option)
		fclose(icode);
#endif
}
#endif

#ifdef PC
void fatal(char *message) {
	debug('f');
	clean_up();
	msg2("Fatal error!\n%s\n", message);
#ifdef PC
	fflush(outstr);
	if (pause_on_error)
		getchar();
	exit(EXIT_FAILURE);
#else
//	while (!kbhit());
	ngetchx();
	exit(0);
#endif
}
#endif
// vim:ts=4:sw=4
