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

int main(int argc,char **argv) {
	char *folder=NULL,*extension=NULL;
	char **p=argv+1;
	int n=argc-1;
	while (n && **p=='-') {
		if (!strcmp(*p,"-f")) {
			folder=p[1];
			n-=2,p+=2;
			continue;
		}
		if (!strcmp(*p,"-e")) {
			extension=p[1];
			n-=2,p+=2;
			continue;
		}
		fprintf(stderr,"unknown option\n");
		return 2;
	}
	if (!n) {
		fprintf(stderr,"usage: dat89y [-f <folder>] [-e <extension>] <file1> ... <fileN>\n");
		return 1;
	}
	if (!extension)
	    extension = "OTH";
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
		int len=0/*content itself*/+1+strlen(extension)+1+1/*estack tag*/;
		unsigned long outlen;
		char c,*dat,*dp;
		FILE *fp=fopen(filename,"rb");
		char *outdat;
		if (!fp) {
			fprintf(stderr,"can't open file '%s'!\n",filename);
			return 3;
		}
		while (getc(fp)>=0)
			len++;
		fseek(fp,0,SEEK_SET);
		dp=dat=malloc(len);
		while (1) {
			int v=getc(fp);
			if (v<0)
			    break;
			*dp++=(c=v);
		}
		*dp++=0;
		{
		    int i;
		    for (i=0;extension[i];i++)
			*dp++=extension[i];
		}
		*dp++=0;
		*dp=0xF8;
		fclose(fp);
		outdat=create_ti_file(0,0x0C,rootname,folder,dat,len,&outlen);
		free(dat);
		memcpy(rootname,basename,strlen(rootname));
		strcat(rootname,".89y");
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
