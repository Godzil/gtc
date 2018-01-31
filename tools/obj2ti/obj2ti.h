// Written by Julien Muchembled.
// Fixed by Romain Lievin for Linux.
// Copyright (c) 2001. All rights reserved.

#if defined(WIN32) || defined(__WIN32__)
# include <windows.h>
#endif

#include <malloc.h>
#include <stdio.h>
#include "str.h"

#include <stdlib.h>

#define _alloca __builtin_alloca


#if !defined(WIN32) && !defined(__WIN32__)
# define _MAX_PATH 1024 //PATH_MAX //unistd.h
# define LOBYTE(w) (unsigned char)(w)
# define HIBYTE(w) (unsigned char)((w) >> 8)
typedef unsigned long DWORD;
#endif

#include "stubs.h"
#include "m68kdefs.h"

#ifndef STYP_TEXT
#define STYP_TEXT	0x20
#endif
#ifndef STYP_DATA
#define STYP_DATA	0x40
#endif
#ifndef STYP_BSS
#define STYP_BSS	0x80
#endif

// Taken from the source of a68k.
#define HunkUnit 999
#define HunkName 1000
#define HunkCode 1001
#define HunkData 1002
#define HunkBSS  1003
#define HunkR32  1004
#define HunkR16  1005
#define HunkR8   1006
#define HunkExt  1007
#define HunkSym  1008
#define HunkDbg  1009
#define HunkEnd  1010

#define TypeEnd 0
#define TypeExport 1
#define TypeImport32 0x81
#define TypeImport16 0x83
#define TypeImport8 0x84
#define TypeImport16Alt 0x8A

enum ObjErr {
	OBJERR_OK,
	OBJERR_Open,
	OBJERR_Read,
	OBJERR_Mem,
	OBJERR_Unknown,
	OBJERR_NbScns,
	OBJERR_Size,
	OBJERR_HunkType,
	OBJERR_R32Hunk,
	OBJERR_ExtHunk,
	OBJERR_ExtType,
	OBJERR_OptHdr,
	OBJERR_ScnType,
	OBJERR_RlcType,
	OBJERR_nbErrTypes
};

enum TI68kErr {
	TI68kERR_OK,
	TI68kERR_NoOutput=OBJERR_nbErrTypes,
	TI68kERR_Unknown,
	TI68kERR_BadExport,
	TI68kERR_Conflict,
	TI68kERR_DoorsOSUndef,
	TI68kERR_NostubUndef,
	TI68kERR_prosit,
	TI68kERR_TooLarge,
	TI68kERR_nbErrTypes
};

char *szErr[TI68kERR_nbErrTypes-1]={
	"Error in opening object file",
	"Error in reading object file",
	"Not enough memory",
	"Unknown format",
	"Bad number of sections",
	"Unexpected end of file",
	"Unsupported hunk type",
	"Unexpected or bad Reloc32 hunk",
	"Unexpected Ext. hunk",
	"Unsupported Ext. type",
	"Optional header is unsupported",
	"Unsupported section type",
	"Unsupported relocation type",
	"No target (_ti89, _ti92plus, ...) is defined",
	"Program format is unsupported or not specified",
	"Exported label is not valid",
	NULL, NULL, NULL,
	"Prosit format is unsupported"
	"Variable too large"
};

enum TI68kType {
	TI68k_none,
	TI68k_68kP,
	TI68k_68kL,
	TI68k_nostub,
	TI68k_nostub_dll,
	TI68k_prosit,
	TI68k_ext,
};

char *szTI68kType[]={
	"_main",
	"_library",
	"_nostub",
	"_nostub_dll",
	"_prosit",
	"_extfile"
};

#define Delete(mem) do{if (mem) {delete mem; mem = NULL;}}while(0)
#define DeleteCast(type,mem) do{if ((type)mem) {delete (type)mem; mem = NULL;}}while(0)

typedef struct {
	char *name;
	unsigned long nrlcs, *rlc;
}	Import;
typedef struct {
	char *name;
	unsigned long value;
}	Export;

class Object
{
private:
	long Size;
	union {
		void *o0;
		unsigned char *o8;
		unsigned long *o32;
		FILHDR *filhdr;
	};
	enum {
		OBJFMT_None,
		OBJFMT_Amiga,
		OBJFMT_COFF
	}	ObjFmt;

protected:
	bool bBadExport;
	struct {
		unsigned long size, *R32[3], nR32[3], nImp16, nImp32, nExports;
		union {
			void *o0;
			unsigned char *o8;
			unsigned long *o32;
		};
		Import *Imp16, *Imp32;
		Export *Exports;
		bool DelDat;
	}	Hunk[3];
	int CodeHunk, DataHunk, BSSHunk;
	unsigned int nscns;

public:
	Object () {
		Size = 0; o0 = NULL; ObjFmt = OBJFMT_None;
		memset (Hunk, 0, sizeof (Hunk)); nscns = 0;
		CodeHunk = DataHunk = BSSHunk = -1;
		bBadExport = false;
	}
	virtual ~Object () {
		int i; for (i=0; i<3; i++) {
			unsigned long j;
			if (Hunk[i].DelDat) Delete (Hunk[i].o8);
			for (j=0; j<3; j++) Delete (Hunk[i].R32[j]);
			for (j=0; j<Hunk[i].nImp16; j++)
				Delete (Hunk[i].Imp16[j].rlc);
			if (Hunk[i].Imp32)
				for (j=0; j<Hunk[i].nImp32; j++)
					Delete (Hunk[i].Imp32[j].rlc);
			Delete (Hunk[i].Imp16);
			Delete (Hunk[i].Imp32);
			Delete (Hunk[i].Exports);
		}
		if (o0) free (o0), o0 = NULL;
	}

	void Merge (int &, int &);
	ObjErr Read (char *);

private:
	void AllocExt (int, Import **, Import **, Export **);
	char *COFF_ReadName (SYMENT *, char *);
	void MergeImp (unsigned long, Import *, unsigned long &, Import * &, unsigned long);
	void Merge2 (unsigned long &, void * &, unsigned long &, void * &, unsigned long);
#ifdef SUPPORT_AMIGA
	ObjErr ReadAmiga ();
#endif
	ObjErr ReadCOFF ();
};

class TI68k : public Object  
{
private:
	char **szErr;
	TI68kType Type;
	union {
		unsigned short Flags;
		struct {
			unsigned short _Output92p    : 1;
			unsigned short _Output89     : 1;
			unsigned short _NoSaveScreen : 1;
			unsigned short _ReadOnly     : 1;
			unsigned short Unused        : 4;
			unsigned short _LibVersion   : 8;
		}	bFlags;
	};
#define Output92p    (bFlags._Output92p)
#define Output89     (bFlags._Output89)
#define NoSaveScreen (bFlags._NoSaveScreen)
#define ReadOnly     (bFlags._ReadOnly)
#define LibVersion   (bFlags._LibVersion)
	unsigned long size;
	unsigned char *dat;

private:
	TI68kErr Conflict (TI68kType);
	TI68kErr DoSave (char *, char *, char *, unsigned long, unsigned short);
	TI68kErr MakeDoorsOS ();
	TI68kErr MakeNostub ();
	TI68kErr MakeExt ();
	bool NostubImp (unsigned long, Import *);
	bool ExtImp (unsigned long, Import *);
	void UndefRef (Import *);

public:
	TI68k (char **szErr2, TI68kType Type = TI68k_none) {
		szErr = szErr2;
		this->Type = Type;
		Flags = 0;
		size = 0; dat = NULL;
	}
	virtual ~TI68k () {
		Delete (dat);
		if (szErr) Delete (szErr[TI68kERR_Conflict-1]);
	}

	TI68kErr Make ();
	TI68kErr Save (char *, bool, bool);
	unsigned long GetExeSize (void) { return size+2; }
};
