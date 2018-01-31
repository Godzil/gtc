// Workspace management routines

typedef char FILENAME[8+1+8+1];
enum { WI_FOLD=0, WI_FILE };
typedef struct {
	int type;	// =WI_FILE
	FILENAME name;
	int open;
	HANDLE openhd;
} WFILE;
#define NUM_DEFLTTYPE 2
typedef struct {
	int type;	// =WI_FOLD
	char title[24];
	int deflttype[NUM_DEFLTTYPE];
	int expanded;
} WFOLD;
typedef union {
	int type;
	WFILE f;
	WFOLD d;
	char pad[32];
} WITEM;
typedef struct {
	// General info
	char prettyname[32];
	char iname[8+1];
	char foldname[8+1];
	int type;
	int loaded;
	// Settings
	int search_attr;
	
	// Content
	int nitems;
	WITEM items[0];
} WSP;
enum { WSPT_SIMPLE=0, WSPT_GAME=1, WSPT_UTIL=2, WSPT_OTHER=3 };

#define WSP_MAX_ITEMS 32
WSP *wsp=0;
char temp_wsp[sizeof(WSP)+WSP_MAX_ITEMS*sizeof(WITEM)];

#include "Files.h"

void WspPromptSave();
int WspAddFold(char *fold);
void WspNew() {
	WSP this_w,*w=&this_w;
	WspPromptSave();
	memset(w,0,sizeof(WSP));
	wzStart("New project","3");
	while (wzTextClr("Project name:",w->prettyname,31))
//	while (wzTextClr("Project internal name:",w->iname,8))
	while (wzTextClr("Project folder:",w->foldname,8)) {
	if (!ValidateSymName(w->foldname)) wzErr("Please enter a valid folder name");
	strtolower(w->foldname);
	while (wzChoiceDeflt("Project type:",
			wzList("Simple program","Game","Utility","Other"),&w->type,WSPT_SIMPLE)) {
		wzDone("Your project is now created. "
			"The project browser (accessible through 2ND-[VAR-LINK]) will now be opened for you.");
		strcpy(w->iname,"project");
		w->loaded=0;
		*(WSP *)temp_wsp=*w;
		wsp=(WSP *)temp_wsp;
		WspAddFold("Default");
		return;
	}
	}
}
void WspShowBrowser();
void WspSave() {
	WSP *w=wsp;
	char tn[20];
	strcpy(tn,w->foldname);
	strcat(tn,"\\");
	strcat(tn,w->iname);
	SYM_STR sym=SYMSTR(tn);
	EM_moveSymFromExtMem(tn,HS_NULL);
	rename(tn,"_wsptemp");
	SYM_STR foldss=SYMSTR(w->foldname);
	if (!SymFindHome(foldss).folder) FolderAdd(foldss);
	FILE *fp=fopen(tn,"wb");
	fwrite(w,sizeof(WSP)+w->nitems*sizeof(WITEM),1,fp);
	fwrite((char[]){0,'W','S','P',0,OTH_TAG},1,6,fp);
	if (ferror(fp)) {
		fclose(fp);
		unlink(tn);
		rename("_wsptemp",tn);
		EM_moveSymToExtMem(tn,HS_NULL);
		SimpleDlg("Save project","Error : the project could not be saved.",B_CENTER,W_NORMAL|ICO_WARN);
		return;
	}
	fclose(fp);
	unlink("_wsptemp");
	if (!EM_moveSymToExtMem(tn,HS_NULL))
		SimpleDlg("Save project",
			"Warning : the project could not be archived. Please go to the [VAR-LINK] "
			"window, delete the files you don't need, and archive it.",B_CENTER,W_NORMAL|ICO_WARN);
	WspShowBrowser();
}
void WspPromptSave() {
	if (wsp) {
		if (YesNoDlg("GT-Dev","Save the current project?"))
			WspSave();
	}
}

int WspFindNearItem(char *wi_name,int wi_type,int i) {
	int n=wsp->nitems-i;
	WITEM *wip=wsp->items+i;
	while (n--) {
		if (wip->type<wi_type) return i;
		if (wip->type==wi_type && strcmp(wip->f.name,wi_name)>=0) return i;
		i++; wip++;
	}
	return i;
}
int WspFindItem(char *wi_name,int wi_type,int i) {
	int j=WspFindNearItem(wi_name,wi_type,i);
	if (j<wsp->nitems && wsp->items[j].type==wi_type && !strcmp(wsp->items[j].f.name,wi_name))
		return j;
	return -1;
}
WITEM *WspAddItem(int i) {
	if (wsp->nitems>=WSP_MAX_ITEMS) return NULL;
	WITEM *wi=wsp->items+i;
	memmove(wi+1,wi,(wsp->nitems-i)*sizeof(WITEM));
	wsp->nitems++;
	memset(wi,0,sizeof(WITEM));
	return wi;
}
int WspAddFile(char *file,char *dfold) {
	int i=WspFindItem(dfold,WI_FOLD,0);
	if (i<0) return 0;
	WFILE *wf=&(WspAddItem(WspFindNearItem(file,WI_FILE,i+1))->f);
	if (!wf) return 0;
	wf->type=WI_FILE;
	strcpy(wf->name,file);
	// everything else is set to 0 :)
	return 1;
}
int WspAddFold(char *fold) {
//	asm("0:bra 0b");
	WFOLD *wd=&(WspAddItem(WspFindNearItem(fold,WI_FOLD,0))->d);
	if (!wd) return 0;
	wd->type=WI_FOLD;
	strcpy(wd->title,fold);
	// everything else is set to 0 :)
	return 1;
}

DEFINE_XPLOOP_CB(callback) {
	if (!key) {
		char *Menu[5]={"Help","New"DOTS,"Import\xA0","Rename","Setup"};
		if (xc->s[xc->sel].i==1) Menu[4]=NULL;
		MenuContents=Menu;
		return XPLCB_CONT;
	} else if (key==KEY_F1) {
		return XPLCB_BREAK;
	}
	return XPLCB_CONT;
}
void WspShowBrowser() {
	int h=8;
	WIN_RECT win;
	XP_C *xc=XpLoadItems(0);
	PushScr();
	dialog("Project browser",120,h+XP_H*XP_N,B_CENTER|B_ROUNDED,&win);
	DStr(win.x0>>2,win.y0,"Project view :");
	xc->sel=0;
	XP_S *xs;
	if ((xs=XpLoop(xc,win.y0+h,callback,NULL))) {
		
	}
	PopScr();
	free(xc);
	MenuContents=NULL;
}
