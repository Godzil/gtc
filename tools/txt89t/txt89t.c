#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void fatal(char *s) {
    fprintf(stderr,"error: %s\n");
    exit(123);
}
char *create_ti_file(int calc,int tigl_type,char *name,char *folder,char *content,unsigned long content_size,unsigned long *ti_file_size);

char *rightest_of(char *s,char c) {
	char *t=strrchr(s,c);
	if (t) return t+1;
	return s;
}
char *leftest_end_of(char *s,char c) {
	char *t=strchr(s,c);
	if (t) return t;
	return s+strlen(s);
}

int compact=0;
char getTIch(FILE *fp,int reset) {
	static int need_space=1;
	int c;
	if (reset) {
		need_space=1;
		return 0;
	}
	if (need_space && need_space--)
		return ' ';
	do
	    c=getc(fp);
	while (c=='\r');
	if (c<0)
		return 0;
	if (!compact && c=='\n') {
		c='\r';
		need_space=1;
	}
	if (c=='\t') {
		c=' ';
		need_space=!compact;
	}
	return c;
}

int main(int argc,char **argv) {
	char *folder=NULL;
	char **p=argv+1;
	int n=argc-1;
	while (n && **p=='-') {
		if (!strcmp(*p,"-f")) {
			folder=p[1];
			n-=2,p+=2;
			continue;
		}
		if (!strcmp(*p,"-c") || !strcmp(*p,"--compact")) {
			compact=1;
			n--,p++;
			continue;
		}
		fprintf(stderr,"unknown option\n");
		return 2;
	}
	if (!n) {
		fprintf(stderr,"usage: txt89t [-f <folder>] [-c|--compact] <file1> ... <fileN>\n");
		return 1;
	}
	while (n--) {
		char *filename=*p++;
		char *basename=rightest_of(rightest_of(filename,'\\'),'/');
		char *rootname_end=leftest_end_of(basename,'.');
		char *rootname=malloc(rootname_end-basename+4+1);
		int i;
		memcpy(rootname,basename,rootname_end-basename);
		rootname[rootname_end-basename]=0;
		for (i=0;i<rootname_end-basename;i++) {
			char c=tolower(rootname[i]);
			if (c=='-' || c==' ') c='_';
			rootname[i]=c;
		}
		{
		int len=2/*cursor pos*/+0/*content itself*/+1/*estack tag*/;
		unsigned long outlen;
		char c,*dat,*dp;
		FILE *fp=fopen(filename,"r");
		char *outdat;
		if (!fp) {
			fprintf(stderr,"can't open file '%s'!\n",filename);
			return 3;
		}
		getTIch(0,1);
		do
			len++;
		while (getTIch(fp,0));
		fseek(fp,0,SEEK_SET);
		getTIch(0,1);
		dp=dat=malloc(len);
		*dp++=0;
		*dp++=1;
		do
			*dp++=(c=getTIch(fp,0));
		while (c);
		*dp=0xE0;
		fclose(fp);
		outdat=create_ti_file(0,0x21,rootname,folder,dat,len,&outlen);
		free(dat);
		memcpy(rootname,basename,strlen(rootname));
		strcat(rootname,".89t");
		fp=fopen(rootname,"wb");
		if (!fp) {
			fprintf(stderr,"can't write file!\n");
			return 3;
		}
		fwrite(outdat,1,outlen,fp);
		fclose(fp);
		free(outdat);
		}
		free(rootname);
	}
	return 0;
}
