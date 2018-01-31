typedef struct {
	int magic[2];
	unsigned int upksize,pksize;
	char escbits,esc0;
	char lzx,rlenum;
	char rlec[0];
} XPAK_HDR;
#define XPAK_MAGIC1 (('G'<<8)+'T')
#define XPAK_MAGIC2 (('P'<<8)+'k')
#define xpak_ismagic(m) (m[0]==XPAK_MAGIC1 && m[1]==XPAK_MAGIC2)
#define xpak_setmagic(m) (void)(m[0]=XPAK_MAGIC1,m[1]=XPAK_MAGIC2)
#header <xupaki>
