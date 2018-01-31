/*
 * GTools C compiler
 * =================
 * source file :
 * (on-calc) flashapp header define's
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define OO_HANDLE (0xFF000000u)
#define OO_SYSTEM_FRAME OO_HANDLE
typedef ULONG pFrame;
typedef HANDLE AppID;

typedef void (* APP_EXT_FUNC)(void);
typedef struct SAppExtension
{
   unsigned long name;
   unsigned long help;
   unsigned short index;
} APP_EXTENSION;
typedef struct SAppExtEntry
{
   APP_EXT_FUNC extension;
   unsigned short flags;
} APP_EXT_ENTRY;
enum {APP_EXT_PROGRAM=0x0000, APP_EXT_FUNCTION=0x0001};
typedef enum
{
   ACB_BUILTIN   =0x0001,
   ACB_INSTALLED =0x0002,
   ACB_LOCALIZER =0x0004,
   ACB_LOCK      =0x0008,
   ACB_JT_VERSION=0x0010,
   ACB_SELECTED  =0x0020,
   ACB_COLLAPSE  =0x0800,
   ACB_BG        =0x1000,
   ACB_COMPRESS  =0x4000,
   ACB_DELETE    =0x8000
} ACB_Flags;

#define MAKE_OO_HANDLE(h) ((h) | OO_HANDLE)
#define OO_GET_HANDLE(h) ((h) & ~OO_HANDLE)
#define IS_OO_HANDLE(h) ((h) > OO_HANDLE)
typedef enum {OO_RW=0,  OO_RO=1,
              OO_SEQ=0, OO_KEYED=2} OO_Flags;
typedef struct
{
   ULONG key;
   void *value;
} OO_Attr;
typedef struct SFrameHdr
{
   pFrame parent;
   pFrame prototype;
   OO_Flags flags;
   ULONG first;
   ULONG count;
} OO_Hdr;
typedef struct SFrame
{
   OO_Hdr head;
   union
   {
      void *value[65000];
      OO_Attr pair[65000];
   } attr;
} Frame;
typedef void (* const OO_MethodPtr)(void);
#define STRING_FRAME(name, parent, proto, first, count) \
const OO_Hdr name =   \
{                     \
   (pFrame)parent,    \
   (pFrame)proto,     \
   OO_RO | OO_SEQ,    \
   first,             \
   count              \
};                    \
static const char * const name##Attr[count] = \
{
#define FRAME(name, parent, proto, first, count) \
const OO_Hdr name =   \
{                     \
   (pFrame)parent,    \
   (pFrame)proto,     \
   OO_RO | OO_KEYED,  \
   first,             \
   count              \
};                    \
static const OO_Attr name##Attr[count] = \
{
#define ATTR(selector, val) {selector, (void *)(val)},
#define STRING_ATTR(sel, s) {OO_FIRST_STRING+(sel), s},
#define ENDFRAME };
#define MAX_APPLET_NAME_SIZE (8)

#define OO_APP_FLAGS (1)
#define GetAppFlags(obj) \
  (APP_Flags)OO_GetAppAttr(obj,1)
#define SetAppFlags(obj,value) \
  OO_SetAppAttr(obj,1,(void *)value)
#define OO_APP_NAME (2)
#define GetAppName(obj) \
  (UCHAR *)OO_GetAppAttr(obj,2)
#define SetAppName(obj,value) \
  OO_SetAppAttr(obj,2,(void *)value)
#define OO_APP_TOK_NAME (3)
#define GetAppTokName(obj) \
  (UCHAR *)OO_GetAppAttr(obj,3)
#define SetAppTokName(obj,value) \
  OO_SetAppAttr(obj,3,(void *)value)
#define OO_APP_PROCESS_EVENT (4)
#define AppProcessEvent(obj,a) \
  ((void (* const)(pFrame, PEvent))OO_GetAttr(obj,4))(obj,a)
#define OO_APP_DEFAULT_MENU (5)
#define GetAppDefaultMenu(obj) \
  (MENU *)OO_GetAppAttr(obj,5)
#define SetAppDefaultMenu(obj,value) \
  OO_SetAppAttr(obj,5,(void *)value)
#define OO_APP_DEFAULT_MENU_HANDLE (6)
#define GetAppDefaultMenuHandle(obj) \
  (HANDLE)OO_GetAppAttr(obj,6)
#define SetAppDefaultMenuHandle(obj,value) \
  OO_SetAppAttr(obj,6,(void *)value)
#define OO_APP_EXT_COUNT (7)
#define GetAppExtCount(obj) \
  (long)OO_GetAppAttr(obj,7)
#define SetAppExtCount(obj,value) \
  OO_SetAppAttr(obj,7,(void *)value)
#define OO_APP_EXTENSIONS (8)
#define GetAppExtensions(obj) \
  (APP_EXTENSION const *)OO_GetAppAttr(obj,8)
#define SetAppExtensions(obj,value) \
  OO_SetAppAttr(obj,8,(void *)value)
#define OO_APP_EXT_ENTRIES (9)
#define GetAppExtEntries(obj) \
  (APP_EXT_ENTRY const *)OO_GetAppAttr(obj,9)
#define SetAppExtEntries(obj,value) \
  OO_SetAppAttr(obj,9,(void *)value)
#define OO_APP_LOCALIZE (10)
#define AppLocalize(obj,a) \
  ((BOOL (* const)(AppID, UCHAR const *))OO_GetAppAttr(obj,10))(obj,a)
#define OO_APP_UNLOCALIZE (11)
#define AppUnlocalize(obj) \
  ((void (* const)(AppID))OO_GetAppAttr(obj,11))(obj)
#define OO_APP_CAN_DELETE (12)
#define AppCanDelete(obj) \
  ((BOOL (* const)(AppID))OO_GetAppAttr(obj,12))(obj)
#define OO_APP_CAN_MOVE (13)
#define AppCanMove(obj) \
  ((BOOL (* const)(AppID))OO_GetAppAttr(obj,13))(obj)
#define OO_APP_VIEWER (14)
#define AppViewer(obj,a,b,c) \
  ((BOOL (* const)(AppID, BYTE *, WINDOW *, HSYM))OO_GetAppAttr(obj,14))(obj,a,b,c)
#define OO_APP_ICON (15)
#define GetAppIcon(obj) \
  (BITMAP *)OO_GetAppAttr(obj,15)
#define SetAppIcon(obj,value) \
  OO_SetAppAttr(obj,15,(void *)value)
#define OO_APP_EXT_HELP (16)
#define AppExtHelp(obj,a) \
  ((void (* const)(AppID, USHORT))OO_GetAppAttr(obj,16))(obj,a)
#define OO_APP_NOTICE_INSTALL (17)
#define AppNoticeInstall(obj,a) \
  ((void (* const)(AppID, ACB const *))OO_GetAppAttr(obj,17))(obj,a)
#define OO_APP_ABOUT (18)
#define AppAbout(obj) \
  ((char const * (* const)(AppID))OO_GetAppAttr(obj,18))(obj)
#define OO_SFONT (768)
#define Getsfont(obj) \
  (SF_CHAR *)OO_GetAttr(obj,768)
#define Setsfont(obj,value) \
  OO_SetAttr(obj,768,(void *)value)
#define OO_LFONT (769)
#define Getlfont(obj) \
  (LF_CHAR *)OO_GetAttr(obj,769)
#define Setlfont(obj,value) \
  OO_SetAttr(obj,769,(void *)value)
#define OO_HFONT (770)
#define Gethfont(obj) \
  (HF_CHAR *)OO_GetAttr(obj,770)
#define Sethfont(obj,value) \
  OO_SetAttr(obj,770,(void *)value)
#define OO_APP_SFONT (768)
#define GetAppSFont(obj) \
  (SF_CHAR *)OO_GetAppAttr(obj,768)
#define SetAppSFont(obj,value) \
  OO_SetAppAttr(obj,768,(void *)value)
#define OO_APP_LFONT (769)
#define GetAppLFont(obj) \
  (LF_CHAR *)OO_GetAppAttr(obj,769)
#define SetAppLFont(obj,value) \
  OO_SetAppAttr(obj,769,(void *)value)
#define OO_APP_HFONT (770)
#define GetAppHFont(obj) \
  (HF_CHAR *)OO_GetAppAttr(obj,770)
#define SetAppHFont(obj,value) \
  OO_SetAppAttr(obj,770,(void *)value)
#define OO_LANGUAGE (784)
#define GetLanguage(obj) \
  (WORD)OO_GetAttr(obj,784)
#define SetLanguage(obj,value) \
  OO_SetAttr(obj,784,(void *)value)
#define OO_DATE_FORMAT (785)
#define GetDateFormat(obj) \
  (char const *)OO_GetAttr(obj,785)
#define SetDateFormat(obj,value) \
  OO_SetAttr(obj,785,(void *)value)
#define OO_BUILTIN_HELP (786)
#define BuiltinHelp(obj,a) \
  ((void (* const)(pFrame, USHORT))OO_GetAttr(obj,786))(obj,a)
#define OO_KTLIST (800)
#define GetKTList(obj) \
  (keytag const *)OO_GetAttr(obj,800)
#define SetKTList(obj,value) \
  OO_SetAttr(obj,800,(void *)value)
#define OO_CAT_TABLE (801)
#define GetCAT_table(obj) \
  (CATALOG const *)OO_GetAttr(obj,801)
#define SetCAT_table(obj,value) \
  OO_SetAttr(obj,801,(void *)value)
#define OO_CAT_INDEX (802)
#define GetCAT_index(obj) \
  (short const *)OO_GetAttr(obj,802)
#define SetCAT_index(obj,value) \
  OO_SetAttr(obj,802,(void *)value)
#define OO_CAT_COUNT (803)
#define GetCAT_count(obj) \
  (short)OO_GetAttr(obj,803)
#define SetCAT_count(obj,value) \
  OO_SetAttr(obj,803,(void *)value)
#define OO_CHAR_MENU (816)
#define GetCharMenu(obj) \
  (const MENU *)OO_GetAttr(obj,816)
#define SetCharMenu(obj,value) \
  OO_SetAttr(obj,816,(void *)value)
#define OO_CHAR_HANDLER (817)
#define CharHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,817))(obj)
#define OO_APPS_HANDLER (818)
#define AppsHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,818))(obj)
#define OO_FLASH_APPS_HANDLER (819)
#define FlashAppsHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,819))(obj)
#define OO_MATH_HANDLER (820)
#define MathHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,820))(obj)
#define OO_MEM_HANDLER (821)
#define MemHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,821))(obj)
#define OO_STO_HANDLER (822)
#define StoHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,822))(obj)
#define OO_QUIT_HANDLER (823)
#define QuitHandler(obj) \
  ((void (* const)(pFrame))OO_GetAttr(obj,823))(obj)

#define OO_FIRST_STRING             2048
#define OO_FIRST_APP_STRING         2048
#define OO_APPSTRING (OO_FIRST_STRING+OO_FIRST_APP_STRING)
#define OO_FIRST_APP_ATTR        0x10000
typedef enum {APP_NONE=0,
              APP_INTERACTIVE=1,
              APP_CON=2,
              APP_ACCESS_SYSVARS=4,
              APP_BACKGROUND=8}
        APP_Flags;

typedef struct SAppHdr
{
   ULONG magic;
   UCHAR name[MAX_APPLET_NAME_SIZE];
   BYTE  zeros[24];
   USHORT flags;
   ULONG dataLen;
   ULONG codeOffset;
   ULONG initDataOffset;
   ULONG initDataLen;
   ULONG optlen;
} AppHdr;
typedef struct SACB
{
   USHORT flags;
   AppID myID;
   AppID next;
   AppID prev;
   ULONG publicstorage;
   AppHdr const *appHeader;
   BYTE const *certhdr;
   pFrame appData;
} ACB;
#define MY_ACB(p) ((ACB*)((BYTE*)&(p)-OFFSETOF(ACB,appData)))
#define MY_APP_ID(p) (MY_ACB(p)->myID)
// vim:ts=4:sw=4
