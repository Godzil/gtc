#define OO_APP_MAGIC (377180989U)
#define OO_HANDLE (0xFF000000u)
#define OO_SYSTEM_FRAME OO_HANDLE
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
#define pFrame _pFrameX
typedef ULONG pFrame;
typedef int AppID;
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
typedef enum {APPHDR_LOCALIZER=0x0001} APPHDR_FLAGS;
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
