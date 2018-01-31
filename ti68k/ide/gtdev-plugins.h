#define PLUGIN_GTBASIC 0
#define PLUGIN_GTC 1
#define PluginName(x) ((x)?"GTC":"GTDEVBAS")

#define ET_FATAL -2
#define ET_WARNING -1
#define ET_ERROR 0
#define ET_INTERNAL_WARNING 1
#define ET_INTERNAL_FAILURE 2
#define et_isinternal(x) ((x)>0)
typedef void CALLBACK (*Msg_Callback_t)(char *message,int err_type,char *func,char *file,int line,int chr);
#define MAX_PROGRESS 65535
typedef void CALLBACK (*Progr_Callback_t)(char *func,char *file,unsigned int fprogress);
#define GTC_Compile ((int(*CALLBACK)(char *in,char *out,Msg_Callback_t msg_process, \
	Progr_Callback_t progr_process)) st->sft[0])

void CALLBACK message_cb(char *message,int err_type,char *func,char *file,int line,int chr);
void CALLBACK progr_cb(char *func,char *file,unsigned int fprogress);

//#define DEBUG_COMPILE
#ifdef DEBUG_COMPILE
#define NEED_DEBUG_MSG
void debug_msg(char *s);
#endif

int Compile(char *in_file,int plugin) {
//#ifdef DEBUG_COMPILE
//	debug_msg("Compile step#1");
//#endif
	SecureTab *st=GetAppSecureTable(PluginName(plugin));
	if (!st) return 0;
//#ifdef DEBUG_COMPILE
//	debug_msg("Compile step#2");
//#endif
	int res=-1;
	if (plugin==PLUGIN_GTC) {
#ifdef COMPILE_ONLY
		ST_helpMsg("Compiling...");
#endif
//#ifdef DEBUG_COMPILE
//		debug_msg("Compile step#3");
//#endif
		res=GTC_Compile(in_file,"main\\outbin",message_cb,progr_cb);
//#ifdef DEBUG_COMPILE
//		{
//		    char b[100];
//		    sprintf(b,"\n Compile step#4: r=%d \n",res);
//		    debug_msg(b);
//		}
//#endif
#ifdef COMPILE_ONLY
		if (!res) ST_helpMsg("Compile successful!");
		else if (res==1) ST_helpMsg("error");
		else if (res==2) ST_helpMsg("couldn't open");
#endif
	}
	return res;
}
