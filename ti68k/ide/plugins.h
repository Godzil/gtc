extern int onopen_gotoline;
void CALLBACK message_cb(char *message,int err_type,char *func,char *file,int line,int chr) {
	char *title="GT-Dev - Error",sprint[340];
	int icon=0;
	strcpy(sprint,"In file '%s' (line %d), in function '%s' :\n\n");
	switch (err_type) {
#define ET_FATAL -2
#define ET_WARNING -1
#define ET_ERROR 0
#define ET_INTERNAL_WARNING 1
#define ET_INTERNAL_FAILURE 2
		case ET_FATAL:
			strcat(sprint,"Fatal error!\n%s");
			icon=ICO_ERROR;
			break;
		case ET_ERROR:
			strcat(sprint,"Error : %s");
			icon=ICO_ERROR;
			break;
		case ET_WARNING:
			strcat(sprint,"Warning : %s");
			icon=ICO_WARN;
			break;
		case ET_INTERNAL_WARNING:
			strcat(sprint,"An unexpected event has occurred (%s); "
					"it might be possible to continue, but the generated code may "
					"contain bugs.\nPlease report this bug to the developer\n\n"
					"Continue?");
			icon=ICO_QUEST;
			break;
		case ET_INTERNAL_FAILURE:
			strcat(sprint,"An internal error has occurred (%s).\n\n"
					"Please report this bug to the developer");
			icon=ICO_ERROR;
			break;
	}
	char sprinted[400];
	if (onopen_gotoline<0) onopen_gotoline=line;
	sprintf(sprinted,sprint,file,line,func,message);
	SimpleDlg(title,sprinted,B_CENTER,W_NORMAL|icon);
}
void CALLBACK progr_cb(char *func,char *file,unsigned int fprogress) {
	char b[100];
	sprintf(b,func?"Function '%s', %d%% of '%s'":"%s%d%% of '%s'",func,fprogress,file);
	b[58]=0;
	char *msg_old=msg;
	msg=b;
	DrawStatus();
	LCD_restore(Port);
	msg=msg_old;
}
