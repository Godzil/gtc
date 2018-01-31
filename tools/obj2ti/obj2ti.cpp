// Written by Julien Muchembled.
// Fixed by Romain Lievin for Linux.
// Further modifications by Sebastian Reichelt, Kevin Kofler and Paul Froissart.
// Copyright (c) 2001-2002. All rights reserved.

// default: don't include AmigaOS support
//#define SUPPORT_AMIGA
// This needs to be defined BEFORE including obj2ti.h, since it is used in
// that header.

#include "obj2ti.h"

// default: don't print out extra debugging information
//#define TEXT_DEBUG

#ifdef TEXT_DEBUG
# define DEBUG_PRINT(str...) (printf (str))
#else
# define DEBUG_PRINT(str...) 
#endif

// default: catch symbols with unknown class
#define CHECK_SYMBOLS

#define MAX_VAR_SIZE 65517

__inline unsigned short br2 (void *pp)
{
	unsigned char *p = (unsigned char *) pp;
	return ((unsigned short) p[0] << 8) + (unsigned short) p[1];
}
__inline unsigned long br4 (void *pp)
{
	unsigned char *p = (unsigned char *) pp;
	return ((unsigned long) p[0] << 24) + ((unsigned long) p[1] << 16)
	       + ((unsigned long) p[2] << 8) + (unsigned long) p[3];
}
__inline void bw2 (void *pp, unsigned short w)
{
	unsigned char *p = (unsigned char *) pp;
	*p++ = (unsigned char)(w>>8);
	*p   = (unsigned char) w;
}
__inline void bw4 (void *pp, unsigned long dw)
{
	unsigned char *p = (unsigned char *) pp;
	*p++ = (unsigned char)(dw>>24);
	*p++ = (unsigned char)(dw>>16);
	*p++ = (unsigned char)(dw>>8);
	*p   = (unsigned char) dw;
}
__inline void ba4 (void *p, unsigned long dw)
{	bw4 (p, br4 (p) + dw); }

__inline unsigned short lr2 (void *pp)
{
	unsigned char *p = (unsigned char *) pp;
	return (unsigned short) p[0] + ((unsigned short) p[1] << 8);
}
__inline unsigned long lr4 (void *pp)
{
	unsigned char *p = (unsigned char *) pp;
	return (unsigned long) p[0] + ((unsigned long) p[1] << 8)
	       + ((unsigned long) p[2] << 16) + ((unsigned long) p[3] << 24);
}
__inline void lw2 (void *pp, unsigned short w)
{
	unsigned char *p = (unsigned char *) pp;
	*p++ = (unsigned char) w;
	*p   = (unsigned char)(w>>8);
}
__inline void lw4 (void *pp, unsigned long dw)
{
	unsigned char *p = (unsigned char *) pp;
	*p++ = (unsigned char) dw;
	*p++ = (unsigned char)(dw>>=8);
	*p++ = (unsigned char)(dw>>=8);
	*p   = (unsigned char)(dw>>8);
}
__inline void la4 (void *p, unsigned long dw)
{	lw4 (p, lr4 (p) + dw); }

void Object::AllocExt (int CurHunk, Import **Imp16, Import **Imp32, Export **Exports)
{
	if (Hunk[CurHunk].nImp16)
		*Imp16 = Hunk[CurHunk].Imp16 = new Import[Hunk[CurHunk].nImp16];
	if (Hunk[CurHunk].nImp32)
		*Imp32 = Hunk[CurHunk].Imp32 = new Import[Hunk[CurHunk].nImp32];
	if (Hunk[CurHunk].nExports)
		*Exports = Hunk[CurHunk].Exports = new Export[Hunk[CurHunk].nExports];
}

#ifdef SUPPORT_AMIGA
ObjErr Object::ReadAmiga ()
{
	int CurHunk=-1; unsigned long *am = o32, size, *Ext[3]={NULL,NULL,NULL};

	bBadExport = true;

	for (size=Size>>2; size>0; size--) {
		DWORD n, HunkType = br4(am++);
		n = br4 (am);
		switch (HunkType) {
			default: return OBJERR_HunkType;
			case HunkUnit:
			case HunkName:
			case HunkDbg:
				size -= ++n; am += n;
			case HunkEnd: break;
			case HunkCode:
				if (CodeHunk >= 0) return OBJERR_NbScns;
				else {
					CodeHunk = ++CurHunk;
					Hunk[CurHunk].size = n << 2;
					Hunk[CurHunk].o32 = ++am;
					am += n; size -= n+1;
				}	break;
			case HunkData:
				if (DataHunk >= 0) return OBJERR_NbScns;
				else {
					DataHunk = ++CurHunk;
					Hunk[CurHunk].size = n << 2;
					Hunk[CurHunk].o32 = ++am;
					am += n; size -= n+1;
				}	break;
			case HunkBSS:
				if (BSSHunk >= 0)  return OBJERR_NbScns;
				else Hunk[BSSHunk = ++CurHunk].size = n << 2, am++, size--;
				break;
			case HunkR16:
				fprintf (stderr, "Warning: 16-bit relocations are unsupported.\n");
				for (; size>0 && n; n=br4 (am))
					n += 2, am += n, size -= n;
				am++; size--; break;
			case HunkR32:
				if (CurHunk < 0) return OBJERR_R32Hunk;
				else {
					for (; size>0 && n; n=br4 (am)) {
						DWORD k = br4 (++am);
						if (k < 3 && !Hunk[CurHunk].R32[k]) {
							unsigned long *R32;
							Hunk[CurHunk].nR32[k] = n;
							Hunk[CurHunk].R32[k] = R32 = new unsigned long [n];
							for (am++; n--; size--) *R32++ = br4 (am++);
							size -= 2;
						}	else return OBJERR_R32Hunk;
					}	am++; size--;
				}	break;
			case HunkExt:
				if (CurHunk < 0 || Ext[CurHunk]) return OBJERR_ExtHunk;
				else {
					unsigned char type; unsigned long nExtRefs=0, nImports=0;
					Ext[CurHunk] = am;
					do {
						n = (br4 (am) & 0xFFFFFF) + 1;
						switch (type = *(unsigned char *)am++) {
							default: return OBJERR_ExtType;
							case TypeEnd: break;
							case TypeImport32:
								Hunk[CurHunk].nImp32++;
							case TypeImport16:
							case TypeImport16Alt:
								am += n; size -= n;
								n = br4 (am-1);
								nImports++;
							case TypeExport:
								am += n; size -= n; nExtRefs++;
					}	}	while (size-- > 0 && type != TypeEnd);
					if (!nExtRefs) Ext[CurHunk] = NULL;
					else {
						Hunk[CurHunk].nExports = nExtRefs - nImports;
						Hunk[CurHunk].nImp16 = nImports - Hunk[CurHunk].nImp32;
	}	}		}	}

	if (size) return OBJERR_Size;
	if (CurHunk < 0) return OBJERR_NbScns;

	nscns = CurHunk+1;
	for (CurHunk=0; (unsigned int)CurHunk<nscns; CurHunk++)
		if ((am = Ext[CurHunk])) {
			unsigned char type;
			Import *Imp16, *Imp32; Export *Exports;
			AllocExt (CurHunk, &Imp16, &Imp32, &Exports);
			while ((type = *(unsigned char *)am) != TypeEnd) {
				unsigned long *rlc, n = br4 (am++) & 0xFFFFFF;
				switch (type) {
					case TypeImport32:
						Imp32->name = (char *)am;
						am += n; n = br4 (am++);
						Imp32->nrlcs = n;
						Imp32->rlc = rlc = new unsigned long [n];
						while (n--) *rlc++ = br4 (am++);
						Imp32++;
						break;
					case TypeImport16:
					case TypeImport16Alt:
						Imp16->name = (char *)am;
						am += n; n = br4 (am++);
						Imp16->nrlcs = n;
						Imp16->rlc = rlc = new unsigned long [n];
						while (n--) *rlc++ = br4 (am++);
						Imp16++;
						break;
					case TypeExport:
						Exports->name = (char *)am; am += n;
						//Exports->size = 0;
						Exports->value = br4 (am++);
						Exports++;
		}	}	}
	return OBJERR_OK;
}
#endif

char *Object::COFF_ReadName (SYMENT *sym, char *Str)
{
	return br4 (sym->e.e.e_zeroes) ?
		sym->e.e_name :
		&Str[br4 (sym->e.e.e_offset)];
}

ObjErr Object::ReadCOFF ()
{
	int CurHunk=BSSHunk=0; SCNHDR *scn = (SCNHDR *)&o8[sizeof(FILHDR)];
	RELOC *Relocs[2]; unsigned short nrlcs[2];
	unsigned long i, nSyms, s_vaddr[3];
	nscns = br2 (filhdr->f_nscns);
	if (nscns < 3) return OBJERR_NbScns;
	if (nscns > 3) {
	 fprintf (stderr, "Warning: More than 3 COFF sections. Excess sections ignored.\n");
	 nscns = 3;
	}
	if (br2 (filhdr->f_opthdr)) return OBJERR_OptHdr;
	s_vaddr[0] = 0;

	for (i=0; i<nscns; i++)
		switch (br4 (scn[i].s_flags) & (STYP_TEXT | STYP_DATA | STYP_BSS)) {
			default: return OBJERR_ScnType;
			case STYP_TEXT | STYP_DATA:
			case STYP_TEXT:
				if (CodeHunk >= 0) return OBJERR_NbScns;
				else {
					Relocs[CurHunk] = (RELOC *)&o8[br4 (scn[i].s_relptr)];
					nrlcs[CurHunk] = br2 (scn[i].s_nreloc);
					Hunk[CodeHunk = ++CurHunk].o8 = &o8[br4 (scn[i].s_scnptr)];
					Hunk[CurHunk].size = br4 (scn[i].s_size);
					s_vaddr[CurHunk] = br4 (scn[i].s_vaddr);
				}
				break;
			case STYP_BSS:
				// This program can only work properly if the BSS section is the last
				// one.
				if (CurHunk != (int) nscns - 1) return OBJERR_NbScns;
				else {
					// Objcopy produces real BSS sections, while the GNU assembler
					// simply leaves an empty section.
					// Both versions will work here, since the relocs are different
					// (section 0 (none) vs. section 3).
					Hunk[0].size = br4 (scn[i].s_size);
					// This is just a stupid number which has to be subtracted from all
					// relocs.
					s_vaddr[0] = br4 (scn[i].s_vaddr);
				}
				break;
			case STYP_DATA:
				if (DataHunk >= 0) return OBJERR_NbScns;
				else {
					Relocs[CurHunk] = (RELOC *)&o8[br4 (scn[i].s_relptr)];
					nrlcs[CurHunk] = br2 (scn[i].s_nreloc);
					Hunk[DataHunk = ++CurHunk].o8 = &o8[br4 (scn[i].s_scnptr)];
					Hunk[CurHunk].size = br4 (scn[i].s_size);
					s_vaddr[CurHunk] = br4 (scn[i].s_vaddr);
				}
				break;
		}

	SYMENT *sym, *Sym = (SYMENT *)&o8[br4 (filhdr->f_symptr)];
	char *Str = (char *)&Sym[nSyms = br4 (filhdr->f_nsyms)];
	struct SYMNFO {
		unsigned char ok;
		unsigned char R32;
		unsigned char IsBSSSymbol;
		unsigned short hunk;
		struct {
			unsigned short n16;
			unsigned short n32;
		}	Hunk[2];
	}	*SymNfo = (SYMNFO *)_alloca (nSyms*sizeof(SYMNFO));
	unsigned long *R32[2][3];
	memset (&R32, 0, sizeof (R32)); memset (SymNfo, 0, nSyms*sizeof(SYMNFO));

/************************************\
|  Basic symbol and relocation info  |
\************************************/

	// Read in the symbol table and build the SymNfo table with additional
	// information.
	// FIXME: Imported symbols shouldn't be considered as exports as well.
	for (sym=Sym,i=0; i<nSyms; sym++,i++) {
		DEBUG_PRINT ("Symbol %ld: Type 0x%lx; Section %ld; Value 0x%lx\n", (long) i, (long) (br2 (sym->e_sclass)), (long) (short) (br2 (sym->e_scnum)), (long) (br4 (sym->e_value)));
#ifdef CHECK_SYMBOLS
		switch (br2 (sym->e_sclass)) {
			// All non-standard symbols are not 'ok'.
			case 0x200:
			case 0x201:
			case 0x300:
			case 0x301:
#endif
				{	unsigned int hunk = br2 (sym->e_scnum);
					if (hunk == nscns) {
						// It is a symbol in the BSS section, as objcopy produces them.
						// This program assumes that the BSS section is the last one.
						// This could be greatly simplified if we assume that there is
						// always just one single symbol (called '.bss') at the start of
						// the BSS section.
						SymNfo[i].IsBSSSymbol = 1;
						SymNfo[i].R32 = 1;
						ba4 (sym->e_value, (DWORD)-(long)s_vaddr[0]);
					}
					if (hunk >= nscns) hunk = 0;
					Hunk[SymNfo[i].hunk = hunk].nExports++;
					SymNfo[i].ok = 1;
				}
#ifdef CHECK_SYMBOLS
		}
#endif
	}

	// Here the number of imports and relocations is read in. No relocation is
	// actually done.
	for (CurHunk=1; (unsigned int)CurHunk<nscns; CurHunk++) {
		unsigned short j; RELOC *rlc = Relocs[CurHunk-1];
		for (j=nrlcs[CurHunk-1]; j; rlc++,j--) {
			unsigned long i = br4 (rlc->r_symndx);
			sym = &Sym[i];
			if (SymNfo[i].ok) {
				unsigned long value = br4 (sym->e_value);
				DEBUG_PRINT ("Reloc %ld: Type 0x%lx; Section %ld; Address 0x%lx\n", (long) (nrlcs[CurHunk-1] - j + 1), (long) (br2 (rlc->r_type)), (long) CurHunk, (long) (long) (br4 (rlc->r_vaddr)));
				DEBUG_PRINT (" Symbol %ld (%s)\n", (long) i, COFF_ReadName (sym, Str));
				switch (br2 (rlc->r_type)) {
					default:
						return OBJERR_RlcType;
					case 0x10:
					case 0x13:
						DEBUG_PRINT ("  16-bit reloc\n");
						if (!value && !SymNfo[i].hunk) {
							DEBUG_PRINT ("   Normal reloc: Section %ld; Address 0x%lx\n", (long) SymNfo[i].hunk, (long) (br4 (sym->e_value)));
							if (!SymNfo[i].Hunk[CurHunk-1].n16++) Hunk[CurHunk].nImp16++;
						}
						break;
					case 0x11:
					case 0x14:
						// Warning: BSSHunk is 0.
						// And 0 is also used for imported symbols, which don't have a
						// section they reside in.
						// Luckily however, we can decide by looking at the value:
						// If it's 0, it's an import, like a ROM_CALL or RAM_CALL.
						// Otherwise, we have a BSS entry. The value then marks the size
						// of the entry, in bytes.
						// That means we simply replace the value by an offset into the
						// BSS section (plus s_vaddr, because COFF requires it). Since
						// this offset can be 0 for the first symbol in the BSS section,
						// we sign a special flag.
						if (SymNfo[i].hunk==0) {
							DEBUG_PRINT ("  Symbol has section 0\n");
							DEBUG_PRINT ("   Value: %ld (marked as BSS: %ld)\n", (long) value, (long) SymNfo[i].IsBSSSymbol);
							if (value || SymNfo[i].IsBSSSymbol) {
								DEBUG_PRINT ("    Inserting reloc into BSS section (0)\n");
								Hunk[CurHunk].nR32[0]++;
								if (!SymNfo[i].R32) {
									unsigned long blocksize = value;
									SymNfo[i].R32 = 1;
									SymNfo[i].IsBSSSymbol = 1;
									bw4 (sym->e_value, value = s_vaddr[0] + Hunk[0].size);
									Hunk[0].size += blocksize;
									DEBUG_PRINT ("    New BSS size: %ld\n", (long) Hunk[0].size);
									DEBUG_PRINT ("    New value for symbol: %ld\n", (long) value);
								}
							} else {
								DEBUG_PRINT ("    Adding import\n");
								if (!SymNfo[i].Hunk[CurHunk-1].n32++) Hunk[CurHunk].nImp32++;
							}
						}	else {
							DEBUG_PRINT ("   Normal reloc: Section %ld; Address 0x%lx\n", (long) SymNfo[i].hunk, (long) (br4 (sym->e_value)));
							Hunk[CurHunk].nR32[SymNfo[i].hunk]++;
							SymNfo[i].R32 = 1;
				}		}
			} else {
				fprintf (stderr, "Warning: Removing reloc to unsupported symbol '%s'.\n", COFF_ReadName (sym, Str));
	}	}	}

/*****************************************************\
| Allocation of export, import, and relocation tables |
\*****************************************************/

	// All tables are built up using the results from the counting above.
	Export *Exports[3]={NULL,NULL,NULL};
	unsigned long **Imp16rlc[3], **Imp32rlc[3];
	for (i=0; i<nscns; i++) {
		Import *Imp16=NULL, *Imp32=NULL;
		// Allocate imports and exports for hunk i using its nImp16 etc. values.
		AllocExt (i, &Imp16, &Imp32, &Exports[i]);
		if (i) {
			unsigned int n16=0, n32=0;
			unsigned long j, **pImp16rlc, **pImp32rlc;
			pImp16rlc = Imp16rlc[i] = Hunk[i].nImp16 ? new unsigned long *[Hunk[i].nImp16] : NULL;
			pImp32rlc = Imp32rlc[i] = Hunk[i].nImp32 ? new unsigned long *[Hunk[i].nImp32] : NULL;
			for (sym=Sym,j=0; j<nSyms; sym++,j++)
				if (!SymNfo[j].hunk) {
					if (SymNfo[j].Hunk[i-1].n16) {
						Imp16->name = COFF_ReadName (sym, Str);
						Imp16->nrlcs = SymNfo[j].Hunk[i-1].n16;
						Imp16->rlc = *pImp16rlc++ = new unsigned long [Imp16->nrlcs];
						Imp16++; SymNfo[j].Hunk[i-1].n16 = ++n16;
					}
					if (SymNfo[j].Hunk[i-1].n32) {
						Imp32->name = COFF_ReadName (sym, Str);
						Imp32->nrlcs = SymNfo[j].Hunk[i-1].n32;
						Imp32->rlc = *pImp32rlc++ = new unsigned long [Imp32->nrlcs];
						Imp32++; SymNfo[j].Hunk[i-1].n32 = ++n32;
				}	}
			// Allocate the relocation tables.
			for (j=0; j<nscns; j++) if (Hunk[i].nR32[j])
				R32[i-1][j] = Hunk[i].R32[j] = new unsigned long [Hunk[i].nR32[j]];
	}	}

/******************************\
| Initialization of all tables |
\******************************/

	// Initialize export tables.
	for (sym=Sym,i=0; i<nSyms; sym++,i++)
		if (SymNfo[i].ok) {
			Exports[SymNfo[i].hunk]->name = COFF_ReadName (sym, Str);
			Exports[SymNfo[i].hunk]->value = br4 (sym->e_value);
			Exports[SymNfo[i].hunk]++;
		}

	// Initialize import and relocation tables, and relocate.
	for (CurHunk=1; (unsigned int)CurHunk<nscns; CurHunk++) {
		unsigned long j; RELOC *rlc = Relocs[CurHunk-1];
		for (j=nrlcs[CurHunk-1]; j; rlc++,j--) {
			unsigned long i = br4 (rlc->r_symndx);
			if (SymNfo[i].ok) {
				int k; unsigned long ofs;
				ofs = br4 (rlc->r_vaddr) - s_vaddr[CurHunk];
				switch (br2 (rlc->r_type)) {
					case 0x10:
					case 0x13:
						if ((k = SymNfo[i].Hunk[CurHunk-1].n16))
							*Imp16rlc[CurHunk][k-1]++ = ofs;
						break;
					case 0x11:
					case 0x14:
						if (SymNfo[i].R32) {
							*R32[CurHunk-1][SymNfo[i].hunk]++ = ofs;
							DEBUG_PRINT ("Relocating 32 bits at offset 0x%lx in section %ld\n", (long) ofs, (long) CurHunk);
							DEBUG_PRINT (" Old value: 0x%lx\n", (long) br4 (&Hunk[CurHunk].o8[ofs]));
							ba4 (&Hunk[CurHunk].o8[ofs], (SymNfo[i].IsBSSSymbol ? br4 (Sym[i].e_value) : 0) + ((DWORD)-(long)s_vaddr[SymNfo[i].hunk]));
							DEBUG_PRINT (" New value: 0x%lx\n", (long) br4 (&Hunk[CurHunk].o8[ofs]));
						} else if ((k = SymNfo[i].Hunk[CurHunk-1].n32))
							*Imp32rlc[CurHunk][k-1]++ = ofs;
		}	}	}
		Delete (Imp16rlc[CurHunk]);
		Delete (Imp32rlc[CurHunk]);
	}

	return OBJERR_OK;
}

ObjErr Object::Read (char *FileName)
{
	long size = 0;
	FILE *f = fopen (FileName, "rb");
	if (!f) return OBJERR_Open;
	fseek (f, 0, SEEK_END);
	Size = ftell (f); rewind (f);
	if ((o0 = malloc (Size)))
		size = fread (o0, 1, Size, f);
	fclose (f);
	if (!o0) return OBJERR_Mem;
	if (size != Size) return OBJERR_Read;

	switch (br2 (o0)) {
#ifdef SUPPORT_AMIGA
		case 0:
			ObjFmt = OBJFMT_Amiga;
			return ReadAmiga ();
#endif
		case 0x150:
			ObjFmt = OBJFMT_COFF;
			return ReadCOFF ();
	}
	return OBJERR_Unknown;
}

void Object::MergeImp (
	unsigned long  nImp1, Import *  Imp1,
	unsigned long &nImp2, Import * &Imp2,
	unsigned long  size)
{
	unsigned long n; Import *imp2 = Imp2;
	for (n=0; n<nImp2; imp2++,n++) {
		unsigned long k, *rlc = imp2->rlc;
		for (k=imp2->nrlcs; k--; rlc++) *rlc += size;
		Import *imp1 = Imp1;
		for (k=nImp1; k--; imp1++)
			if (!strcmp (imp1->name, imp2->name)) {
				unsigned long i, j;
				i = imp1->nrlcs;  j = imp2->nrlcs;
				rlc = new unsigned long [i+j];
				memcpy (rlc, imp1->rlc, i * sizeof (long));
				memcpy (&rlc[i], imp2->rlc, j * sizeof (long));
				Delete (imp1->rlc); Delete (imp2->rlc);
				imp1->rlc = rlc; imp1->nrlcs += j;
				if (!--nImp2) Delete (Imp2);
				else {
					memmove (imp2, imp2+1, (nImp2-n--) * sizeof (Import));
					imp2--;
				}	break;
	}		}
}

void Object::Merge2 (
	unsigned long &n1, void * &p1,
	unsigned long &n2, void * &p2,
	unsigned long SizeOf)
{
	if (n2) {
		unsigned char *p = new unsigned char [(n1+n2)*SizeOf];
		memcpy (p, p1, n1 * SizeOf);
		memcpy (&p[n1*SizeOf], p2, n2 * SizeOf);
		DeleteCast (unsigned char **, p1);
		DeleteCast (unsigned char **, p2);
		n1 += n2; n2 = 0;
		p1 = p;
	}
}

void Object::Merge (int &First, int &Second)
{
	int first, second;
	if ((unsigned int)(second=Second) < nscns) {
		if ((unsigned int)(first=First) < nscns) {
			int hunk; unsigned long size = Hunk[first].size;

			for (hunk=0; hunk<(int)nscns; hunk++) if (hunk != second) {
				unsigned long i, *R32 = Hunk[second].R32[hunk];
				for (i=Hunk[second].nR32[hunk]; i--; R32++) *R32 += size;
			}
			MergeImp (
				Hunk[first].nImp16,  Hunk[first].Imp16,
				Hunk[second].nImp16, Hunk[second].Imp16,
				size);
			MergeImp (
				Hunk[first].nImp32,  Hunk[first].Imp32,
				Hunk[second].nImp32, Hunk[second].Imp32,
				size);
			{
				unsigned long i; Export *exp = Hunk[second].Exports;
				for (i=Hunk[second].nExports; i--; exp++) exp->value += size;
			}

			for (hunk=0; hunk<(int)nscns; hunk++) {
				unsigned long i, j, *R32_1, *R32_2;
				i = Hunk[hunk].nR32[first];  j = Hunk[hunk].nR32[second];
				Hunk[hunk].nR32[first] += j; Hunk[hunk].nR32[second] = 0;
				R32_1 = new unsigned long [i+j];
				memcpy (R32_1, Hunk[hunk].R32[first], i*sizeof(long));
				Delete (Hunk[hunk].R32[first]);
				Hunk[hunk].R32[first] = R32_1; R32_1 += i;
				R32_2 = Hunk[hunk].R32[second];
				for (; j--; R32_1++) {
					ba4 (&Hunk[hunk].o8[*R32_1 = *R32_2++], size);
					if (hunk == second) *R32_1 += size;
				}
				Delete (Hunk[hunk].R32[second]);
			}

			for (hunk=0; hunk<(int)nscns; hunk++) if (hunk != second) {
				unsigned long i, j, *R32;
				i = Hunk[first].nR32[hunk];  j = Hunk[second].nR32[hunk];
				Hunk[first].nR32[hunk] += j; Hunk[second].nR32[hunk] = 0;
				R32 = new unsigned long [i+j];
				memcpy (R32, Hunk[first].R32[hunk], i*sizeof(long));
				memcpy (&R32[i], Hunk[second].R32[hunk], j*sizeof(long));
				Delete (Hunk[first].R32[hunk]);
				Delete (Hunk[second].R32[hunk]);
				Hunk[first].R32[hunk] = R32;
			}

			Merge2 (Hunk[first].nImp16,   (void * &) Hunk[first].Imp16,   Hunk[second].nImp16,   (void * &) Hunk[second].Imp16,   sizeof (Import));
			Merge2 (Hunk[first].nImp32,   (void * &) Hunk[first].Imp32,   Hunk[second].nImp32,   (void * &) Hunk[second].Imp32,   sizeof (Import));
			Merge2 (Hunk[first].nExports, (void * &) Hunk[first].Exports, Hunk[second].nExports, (void * &) Hunk[second].Exports, sizeof (Export));

			unsigned size2 = Hunk[second].size;
			Hunk[first].size += size2; Hunk[second].size = 0;
			unsigned char *o8 = new unsigned char [size + size2];
			memcpy (o8, Hunk[first].o0, size);
			memcpy (&o8[size], Hunk[second].o0, size2);
			if (Hunk[second].DelDat)
				DeleteCast (unsigned char **, Hunk[second].o0);
			if (Hunk[first].DelDat)
				DeleteCast (unsigned char **, Hunk[first].o0);
			else Hunk[first].DelDat = true;
			Hunk[first].o8 = o8;
		}	else First = second, Second = first;
	}
}

#define CONFLICT "%s is incompatible with %s"
TI68kErr TI68k::Conflict (TI68kType Type2)
{
	if (szErr) {
		Delete (szErr[TI68kERR_Conflict-1]);
		char *msg = szErr[TI68kERR_Conflict-1] = new char [
				sizeof(CONFLICT) - 4
			+	strlen(szTI68kType[Type2-1])
			+	strlen(szTI68kType[Type-1])];
		sprintf (msg, CONFLICT, szTI68kType[Type2-1],  szTI68kType[Type-1]);
	}	return TI68kERR_Conflict;
}

void TI68k::UndefRef (Import *imp)
{
	fprintf (stderr, "Error: %lu undefined reference(s) to '%s'.\n", imp->nrlcs, imp->name);
}

enum { RAM32, eRAM32, RAM16, eRAM16, ROM, LIBS };
TI68kErr TI68k::MakeDoorsOS ()
{
	bool bErr=false; Import *imp;
	int i, *lens, n16, n32, *ndx, nExtra16=0, nLibs=LIBS, nRAM16=0; 
	n16 = (signed)Hunk[CodeHunk].nImp16;
	n32 = (signed)Hunk[CodeHunk].nImp32;
	lens = (int *)_alloca (n32 * sizeof (int));
	ndx = (int *)_alloca (n32 * sizeof (int));
	for (imp=Hunk[CodeHunk].Imp32,i=0; i<n32; imp++,i++) {
		char *p = strrchr (&imp->name[1], '@');
		if ((!p) && ((*imp->name)!='_') && strncmp(imp->name,"L__",3)) {
			p = strstr (&imp->name[1], "__");
		}
		if (p) {
			lens[i] = p - imp->name;
			if (lens[i] > 8 && strncmp (imp->name, "_extraramaddr@", 14))
				lens[i] = 0;
		} else if (!strncmp (imp->name, "_RAM_CALL_", 10)
			      || !strncmp (imp->name, "_ROM_CALL_", 10))
			lens[i] = 9;
		else if (!strncmp (imp->name, "_extraramaddr_", 14))
			lens[i] = 13;
		else lens[i] = 0;
		if (!lens[i]) UndefRef (imp), bErr = true;
		else {
			int j; Import *imp2 = Hunk[CodeHunk].Imp32;
			for (j=0; j<i; imp2++, j++)
				if (lens[i] == lens[j])
					if (!strncmp (imp2->name, imp->name, lens[i]))
						lens[i] = -lens[i], ndx[i] = ndx[j], j = i;
			if (j == i) {
				if (lens[i] <= 8) ndx[i] = nLibs++;
				else switch (imp->name[2]) {
					case 'A': ndx[i] = RAM32; break;
					case 'O': ndx[i] = ROM; break;
					default: ndx[i] = eRAM32;
	}	}	}	}

	for (imp = Hunk[CodeHunk].Imp16,i=0; i<n16; imp++, i++) {
		if (!strncmp (imp->name, "_extraramaddr", 13)) nExtra16++;
		else if (!strncmp (imp->name, "_RAM_CALL_", 10)) nRAM16++;
		else UndefRef (imp), bErr = true;
	}

	if (bErr) return TI68kERR_DoorsOSUndef;

	struct LibRef {
		unsigned short ord, nrlcs;
		unsigned long *rlc;
	}	**Refs = (LibRef **)_alloca (nLibs * sizeof (LibRef *));
	struct Lib {
		char name[9];
		unsigned char minVersion /*__attribute__ ((packed))*/;
		union {
			unsigned short nRefs;
			unsigned short nExps;
		};
		union {
			LibRef *Refs;
			unsigned short *Exps;
		};
	}	Exp, *Libs = (Lib *)_alloca (nLibs * sizeof (Lib));
	memset (Libs, 0, nLibs * sizeof (Lib));
	Libs[RAM16].nRefs = nRAM16; Libs[eRAM16].nRefs = nExtra16;

	struct {
		unsigned short code, comment, main, exit, extra;
	}	Ofs = { 0, 0, 0, 0, 0};
	unsigned short HeaderSize = 40 + (nLibs-LIBS) * 12 + 2 *
		(unsigned short)(Hunk[CodeHunk].nR32[CodeHunk] + Hunk[CodeHunk].nR32[BSSHunk]);
	if (!(Hunk[BSSHunk].size)) HeaderSize-=6;

	for (imp=Hunk[CodeHunk].Imp32,i=0; i<n32; imp++,i++) {
		if (lens[i] > 0 && ndx[i] >= LIBS)
			strncpy (Libs[ndx[i]].name, imp->name, lens[i]);
		Libs[ndx[i]].nRefs++;
	}
	if (Libs[ROM].nRefs) HeaderSize += 2;
	if (Libs[RAM32].nRefs || Libs[eRAM32].nRefs
	 || Libs[RAM16].nRefs || Libs[eRAM16].nRefs)
		HeaderSize += 2;
	for (i=0; i<nLibs; i++) {
		Libs[i].Refs = Refs[i]
		= (LibRef *)_alloca (Libs[i].nRefs * sizeof (LibRef));
		HeaderSize += Libs[i].nRefs * 4;
	}
	for (imp=Hunk[CodeHunk].Imp32,i=0; i<n32; imp++,i++) {
		char *p; LibRef **refs = &Refs[ndx[i]];
		(*refs)->ord = (unsigned short)
			strtoul (&imp->name[abs(lens[i])+(((imp->name[abs(lens[i])]=='_')&&(imp->name[abs(lens[i])+1]=='_'))?2:1)], &p, 16);
		HeaderSize += ((*refs)->nrlcs = (unsigned short) imp->nrlcs) * 2;
		(*refs)->rlc = imp->rlc; (*refs)++;
	}

	nRAM16 = nExtra16 = 0;
	for (imp = Hunk[CodeHunk].Imp16,i=0; i<n16; imp++, i++) {
		LibRef *refs; int len; char *p;
		if (!strncmp (imp->name, "_RAM_CALL_", 10))
			len = 10, refs = &Libs[RAM16].Refs[nRAM16++];
		else len = 14, refs = &Libs[eRAM16].Refs[nExtra16++];
		refs->ord = (unsigned short)strtoul (&imp->name[len], &p, 16);
		if (*p) UndefRef (imp), bErr = true;
		HeaderSize += (refs->nrlcs = (unsigned short) imp->nrlcs) * 2;
		refs->rlc = imp->rlc;
	}

	if (bErr) return TI68kERR_DoorsOSUndef;

	Export *exp = Hunk[CodeHunk].Exports; Exp.nRefs = 0;
	for (i=(int)Hunk[CodeHunk].nExports; i--; exp++) {
		if (!strcmp (exp->name, "_comment"))
			Ofs.comment = (unsigned short)exp->value;
		else if (!strcmp (exp->name, "_main"))
			Ofs.main = (unsigned short)exp->value;
		else if (!strcmp (exp->name, "_exit"))
			Ofs.exit = (unsigned short)exp->value;
		else if (!strcmp (exp->name, "_extraram"))
			Ofs.extra = (unsigned short)exp->value;
		else {
			char *p = strrchr (&exp->name[1], '@');
			if ((!p) && ((*exp->name)!='_') && strncmp(exp->name,"L__",3)) {
				p = strstr (&exp->name[1], "__");
			}
			if (p) {
				if (!strncmp(p+((*p=='_')?2:1),"version",7)) {
				int len = p - exp->name;
				if (len > 8) return TI68kERR_BadExport;
				unsigned long j = strtoul (p+((*p=='_')?9:8), &p, 16);
				if ((*p) || (j > 255ul)) return TI68kERR_BadExport; else {
				for (int ii=0; ii<nLibs; ii++) {
					if (!strncmp(Libs[ii].name,exp->name,len)) Libs[ii].minVersion=j;
				}
				}} else {
				int len = p - exp->name;
				if (len > 8) return TI68kERR_BadExport;
				if (!Exp.nRefs) {
					strncpy (Exp.name, exp->name, len);
					Exp.name[len] = 0;
				}	else if (strncmp (Exp.name, exp->name, len))
					return TI68kERR_BadExport;
				Exp.nRefs++;
				}
	}	}	}

	if (Exp.nRefs) {
		HeaderSize += 4 + 2 * Exp.nRefs;
		Exp.Refs = (LibRef *)_alloca (Exp.nRefs * sizeof (short));
		memset (Exp.Refs, -1, Exp.nRefs * sizeof (short));

		exp = Hunk[CodeHunk].Exports;
		for (i=(int)Hunk[CodeHunk].nExports; i--; exp++) {
			char *p = strrchr (&exp->name[1], '@');
			if ((!p) && ((*exp->name)!='_') && strncmp(exp->name,"L__",3)) {
				p = strstr (&exp->name[1], "__");
				if (p) p++;
			}
			if (p++) {
				if (strncmp(p,"version",7)) {
				unsigned long j = strtoul (p, &p, 16);
				if (*p || j >= Exp.nExps || (short)Exp.Exps[j] != -1)
					return TI68kERR_BadExport;
				Exp.Exps[j] = (unsigned short)exp->value;
				}
	}	}	}

	union {
		void *o0;
		unsigned char *o8;
		unsigned short *o16;
		unsigned long *o32;
	};

	Ofs.code = HeaderSize + (Type == TI68k_68kP ? sizeof (PrgmStub) : sizeof (LibStub));
	o8 = dat = new unsigned char [size = Ofs.code + Hunk[CodeHunk].size + 3];

	bw2 (o16++, 0x6100);
	bw2 (o16++, HeaderSize-2);
	bw4 (o32++, ('6'<<24)+('8'<<16)+('k'<<8)+(Type==TI68k_68kP?'P':'L'));
	*o16++ = 0;
	bw2 (o16++, Ofs.comment ? Ofs.comment + Ofs.code : 0);
	bw2 (o16++, Type == TI68k_68kP ? Ofs.main + Ofs.code : 0);
	bw2 (o16++, Ofs.exit ? Ofs.exit + Ofs.code : 0);
	bw2 (o16++, Flags);
	*o16++ = 0; *o32++ = 0;
	bw2 (o16++, Ofs.extra ? Ofs.extra + Ofs.code : 0);

	bw2 (o16++, nLibs-LIBS);
	for (i=LIBS; i<nLibs; o8+=10,i++)
		memcpy (o0, Libs[i].name, 10);
	for (i=LIBS; i<nLibs; i++) {
		int j = Libs[i].nRefs; bw2 (o16++, --j);
		LibRef *Refs = Libs[i].Refs;
		do {
			bw2 (o16++, Refs->ord);
			unsigned long *rlc = Refs->rlc;
			unsigned short k=Refs++->nrlcs;
			while (k--) bw2 (o16++, Ofs.code + (unsigned short)*rlc++);
			*o16++ = 0;
		}	while (j--);
	}

	if (!Libs[ROM].nRefs) *o16++ = 0;
	else {
		bw2 (o16++, (unsigned short)-1);
		int j = Libs[ROM].nRefs; bw2 (o16++, --j);
		LibRef *Refs = Libs[ROM].Refs;
		do {
			bw2 (o16++, Refs->ord);
			unsigned long *rlc = Refs->rlc;
			unsigned short k=Refs++->nrlcs;
			while (k--) bw2 (o16++, Ofs.code + (unsigned short)*rlc++);
			*o16++ = 0;
		}	while (j--);
	}

	i = Libs[RAM32].nRefs + Libs[eRAM32].nRefs
	  + Libs[RAM16].nRefs + Libs[eRAM16].nRefs;
	if (!i--) *o16++ = 0;
	else {
		bw2 (o16++, (unsigned short)-1); bw2 (o16++, i);
		for (i=RAM32; i<=eRAM16; i++) {
			int j; LibRef *Refs = Libs[i].Refs;
			for (j=Libs[i].nRefs; j; j--) {
				bw2 (o16++, Refs->ord + (unsigned short)(i<<14));
				unsigned long *rlc = Refs->rlc;
				unsigned short k=Refs++->nrlcs;
				while (k--) bw2 (o16++, Ofs.code + (unsigned short)*rlc++);
				*o16++ = 0;
	}	}	}

	unsigned long *R32 = Hunk[CodeHunk].R32[CodeHunk];
	for (i=(int)Hunk[CodeHunk].nR32[CodeHunk]; i; i--)
		bw2 (o16++, Ofs.code + (unsigned short)*R32++);
	*o16++ = 0;

	if (Hunk[BSSHunk].size) {
	bw2 (&dat[0x14], (unsigned short)(o8-dat));
	bw4 (o32++, Hunk[BSSHunk].size);
	R32 = Hunk[CodeHunk].R32[BSSHunk];
	for (i=(int)Hunk[CodeHunk].nR32[BSSHunk]; i; i--)
		bw2 (o16++, Ofs.code + (unsigned short)*R32++);
	*o16++ = 0;
	} else bw2 (&dat[0x14], (unsigned short)0);

	if (Exp.nRefs) {
		bw2 (&dat[0x16], (unsigned short)(o8-dat));
		bw2 (o16++, Exp.nRefs);
		for (i=0; i<Exp.nRefs; i++)
			bw2 (o16++, Ofs.code + (unsigned short)Exp.Exps[i]);
		*o16++ = 0;
	}

	if (Type == TI68k_68kP)
		memcpy (o0, PrgmStub, sizeof (PrgmStub)), o8 += sizeof (PrgmStub);
	else memcpy (o0, LibStub, sizeof (LibStub)), o8 += sizeof (LibStub);

	memcpy (o0, Hunk[CodeHunk].o0, Hunk[CodeHunk].size);
	o8 += Hunk[CodeHunk].size;

	R32 = Hunk[CodeHunk].R32[CodeHunk];
	for (i=(int)Hunk[CodeHunk].nR32[CodeHunk]; i; i--)
		ba4 (&dat[Ofs.code+(unsigned short)*R32++], Ofs.code);

	*o16++ = 0; *o8 = 0xF3;

	return TI68kERR_OK;
}

bool TI68k::NostubImp (unsigned long n, Import *imp)
{
	if (!n) return false;
	for (; n--; imp++) UndefRef (imp);
	return true;
}

TI68kErr TI68k::MakeNostub ()
{
	if (BSSHunk >= 0) if (Hunk[BSSHunk].size) {
		Hunk[BSSHunk].o8 = new unsigned char [Hunk[BSSHunk].size];
		Hunk[BSSHunk].DelDat = true;
		memset (Hunk[BSSHunk].o8, 0, Hunk[BSSHunk].size);
		Merge (CodeHunk, BSSHunk);
	}
	if (!NostubImp (Hunk[CodeHunk].nImp16, Hunk[CodeHunk].Imp16)
	 && !NostubImp (Hunk[CodeHunk].nImp32, Hunk[CodeHunk].Imp32)) {
		unsigned long n = Hunk[CodeHunk].nR32[CodeHunk];
		unsigned long *R32 = Hunk[CodeHunk].R32[CodeHunk];
		size = Hunk[CodeHunk].size + 4 * n + 3;
		dat = new unsigned char [size];
		memcpy (dat, Hunk[CodeHunk].o0, Hunk[CodeHunk].size);
		unsigned short *o16 = (unsigned short *)&dat[size-1];
		*(unsigned char *)o16-- = 0xF3;
		for (; n--; R32++) {
			bw2 (o16--, (unsigned short)*R32);
			bw2 (o16--, (unsigned short)br4 (&dat[*R32]));
		}	*o16 = 0;
		return TI68kERR_OK;
	}	else return TI68kERR_NostubUndef;
}

bool TI68k::ExtImp (unsigned long n, Import *imp)
{
	if (!n) return false;
	for (; n--; imp++) UndefRef (imp);
	return true;
}

char *OutputName2=0;
bool InList(char *str, char *list) {
	if (!*list) return true;
	while (*list)
		if (!strcmp(str,list)) return true;
		else while (*list++);
	return false;
}

void maj_conv(char *s) {
	char c;
	while ((c=*s++))
		if (c>='A' && c<='Z')
			memmove(s+1,s,strlen(s)+1), *s='-';
}

TI68kErr TI68k::MakeExt ()
{
	/* TODO (?) : BSS aren't supported */
	if (BSSHunk >= 0) if (Hunk[BSSHunk].size) {
		Hunk[BSSHunk].o8 = new unsigned char [Hunk[BSSHunk].size];
		Hunk[BSSHunk].DelDat = true;
		memset (Hunk[BSSHunk].o8, 0, Hunk[BSSHunk].size);
		Merge (CodeHunk, BSSHunk);
	}
	char *ExportList;
	FILE *ex_fp=fopen("export.dat","rb");
	if (!ex_fp) ExportList="";
	else ExportList = new char [300000], fread(ExportList,300000,1,ex_fp), fclose(ex_fp);
	FILE *def_fp=fopen("deflt.txt","a+");
	unsigned long nExp = 0, szExpN = 0;
	int n=Hunk[CodeHunk].nExports; Export *exp = Hunk[CodeHunk].Exports;
	unsigned long *expOffs = new unsigned long [n]; char **expName = new char * [n];
	for (; n--; exp++) {
		if (InList(exp->name, ExportList)) {
			char buf[200];
			sprintf(buf,"%s.ref",exp->name);
			maj_conv(buf);
			FILE *fp=fopen(buf,"w");
			if (!fp) { printf("Couldn't open '%s' !\n",buf); return TI68kERR_Unknown; }
			sprintf(buf,"%s.ext",OutputName2);
			fputs(buf,fp);
			fclose(fp);
			if (def_fp) fprintf(def_fp,"#var %s\n",exp->name);
			expOffs[nExp] = exp->value, expName[nExp] = exp->name,
				szExpN += strlen(exp->name)+1, nExp++;
		}
	}
	if (def_fp) fclose(def_fp);
	unsigned long nRef0 = 0, nRef = 0, nImp = 0, szImpN = 0; int i;
	int n32=Hunk[CodeHunk].nImp32, n16=Hunk[CodeHunk].nImp16;
	Import *imp32 = Hunk[CodeHunk].Imp32, *imp16 = Hunk[CodeHunk].Imp16, *imp;
	for (n=n16,imp=imp16; n--; imp++)
		nRef0+=imp->nrlcs+1;
	for (n=n32,imp=imp32; n--; imp++)
		nRef0+=imp->nrlcs+1;
	unsigned long *impOffs = new unsigned long [nRef0];
	char **impName = new char * [n16+n32];
	for (n=n16,imp=imp16; n--; imp++) {
		for (i=imp->nrlcs;i--;)
			impOffs[nRef] = imp->rlc[i]-2, nRef++;
		impOffs[nRef] = -2ul, nRef++;
		impName[nImp] = imp->name, szImpN += strlen(imp->name)+1, nImp++;
	}
	for (n=n32,imp=imp32; n--; imp++) {
		for (i=imp->nrlcs;i--;)
			impOffs[nRef] = imp->rlc[i], nRef++;
		impOffs[nRef] = -2ul, nRef++;
		impName[nImp] = imp->name, szImpN += strlen(imp->name)+1, nImp++;
	}
	unsigned long nInt = Hunk[CodeHunk].nR32[CodeHunk];
	unsigned long *R32 = Hunk[CodeHunk].R32[CodeHunk];
	size = (2+Hunk[CodeHunk].size) + (2*nInt+2) + (2*nExp+2+szExpN)		+ (szExpN&1) + (2*nRef+2+szImpN) + (szImpN&1) + (2);
	dat = new unsigned char [size];
	memset (dat, 0, size);
	unsigned char *p8 = dat;
#define w16(x) (bw2(p8,(unsigned short)(x)), p8+=2)
	w16(2+Hunk[CodeHunk].size);
	memcpy (p8, Hunk[CodeHunk].o0, Hunk[CodeHunk].size), p8+=Hunk[CodeHunk].size;
	/* Relocation section */
	for (n=nInt; n--; R32++) {
		w16(2+(unsigned short)*R32);
	}
	w16(0);
	/* Export section */
	for (n=nExp;n--;)
		w16(2+expOffs[n]);
	w16(0);
	for (n=nExp;n--;)
		strcpy((char *)p8, expName[n]), p8+=strlen(expName[n])+1;
	if (szExpN&1) *p8++=0;
	/* Import section */
	for (i=0;i<(int)nRef;i++)
		w16(2+impOffs[i]);
	w16(0);
	for (i=0;i<(int)nImp;i++)
		strcpy((char *)p8, impName[i]), p8+=strlen(impName[i])+1;
	if (szImpN&1) *p8++=0;
	/* BSS section */
	w16(0);
#ifndef NDEBUG
	if (p8!=&dat[size]) { printf("Size mismatch!\n"); return TI68kERR_Unknown; }
#endif
	return TI68kERR_OK;
}

TI68kErr TI68k::Make ()
{
	int hunk;

	for (hunk=0; hunk<3; hunk++) {
		unsigned long i; Export *exp = Hunk[hunk].Exports;
		for (i=Hunk[hunk].nExports; i; exp++,i--) {
			if (!strcmp (exp->name, szTI68kType[TI68k_68kL-1])) {
				if (Type == TI68k_none) Type = TI68k_68kL;
				else return Conflict (TI68k_68kL);
			}
			else if (!strcmp (exp->name, szTI68kType[TI68k_68kP-1])) {
				if (Type == TI68k_none) Type = TI68k_68kP;
				else if (Type != TI68k_nostub)
					return Conflict (TI68k_68kP);
			}
			else if (!strcmp (exp->name, szTI68kType[TI68k_ext-1])) {
				if (Type == TI68k_none) Type = TI68k_ext;
				else
					return Conflict (TI68k_ext);
			}
			else if (!strcmp (exp->name, szTI68kType[TI68k_nostub-1])) {
				if (Type == TI68k_none || Type == TI68k_68kP)
					Type = TI68k_nostub;
				else if (Type != TI68k_nostub_dll)
					return Conflict (TI68k_nostub);
			}
			else if (!strcmp (exp->name, szTI68kType[TI68k_nostub_dll-1])) {
				if (Type == TI68k_none || Type == TI68k_68kP || Type == TI68k_nostub)
					Type = TI68k_nostub_dll;
				else return Conflict (TI68k_nostub_dll);
			}
			else if (!strcmp (exp->name, szTI68kType[TI68k_prosit-1]))
				return TI68kERR_prosit;
			else if (!strcmp (exp->name, "_ti89"))
				Output89 = 1;
			else if (!strcmp(exp->name, "_ti92plus"))
				Output92p = 1;
			else if (!strncmp(exp->name, "_flag_", 6))
				Flags |= 1 << strtoul (&exp->name[6], NULL, 10);
			else if (!strncmp(exp->name, "_version", 6))
				LibVersion = strtoul (&exp->name[8], NULL, 16);
			else if (bBadExport
			 && strcmp (exp->name, "_comment")
			 && strcmp (exp->name, "_exit")
			 && strcmp (exp->name, "_extraram")) {
				char *p = strrchr (&exp->name[1], '@');
				if (p) if (p[1]) {
					strtoul (&p[1], &p, 16);
					if (*p) p = NULL;
				}
				if (!p) return TI68kERR_BadExport;
	}	}	}

	if (!Output89 && !Output92p && Type!=TI68k_ext) return TI68kERR_NoOutput;

	Merge (CodeHunk, DataHunk);
	switch (Type) {
		case TI68k_ext:
			return MakeExt ();
		case TI68k_nostub:
		case TI68k_nostub_dll:
			return MakeNostub ();
		default:
			return MakeDoorsOS ();
	}
	return TI68kERR_Unknown;
}

TI68kErr TI68k::DoSave (char *FileName, char *ext, char *Header, unsigned long HeaderSize, unsigned short chksum)
{
	if (strlen (FileName) + strlen (ext) < _MAX_PATH) {
		unsigned char buf[2]; char FullName[_MAX_PATH]; FILE *f;
		f = fopen (strcat (strcpy (FullName, FileName), ext), "wb");
		if (HeaderSize) fwrite (Header, 1, HeaderSize, f);
		bw2 (buf, (unsigned short)size);
		fwrite (buf, 1, 2, f);
		fwrite (dat, 1, size, f);
		if (HeaderSize) lw2 (buf, chksum), fwrite (buf, 1, 2, f);
		fclose (f);
	}
	return TI68kERR_OK;
}

TI68kErr TI68k::Save (char *FileName, bool outputbin, bool quiet)
{
	static char Header[]=
	"**TI92P*\001\0main\0\0\0\0"
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	"\001\0R\0\0\0\0\0\0\0\0\0\0\0!\0\0\0\0\0\0\0¥Z\0\0\0";
#define HeaderSize (sizeof (Header))

	char AMSname[9], *p; unsigned short chksum;
	if (GetExeSize () > MAX_VAR_SIZE)
		return TI68kERR_TooLarge;
	AMSname[8] = 0; p = strrchr (FileName, '\\');
	_strlwr (strncpy (AMSname, p ? p + 1 : FileName, 8));
	strncpy (&Header[0x40], AMSname, 8);
	lw4 (&Header[0x4C], size + HeaderSize + 4);

	if (outputbin || Type == TI68k_nostub_dll || Type == TI68k_ext) {
		if (Type == TI68k_nostub_dll && !outputbin)
			DoSave (FileName, ".ndl", NULL, 0, 0);
		else if (Type == TI68k_ext && !outputbin)
			DoSave (FileName, ".ext", NULL, 0, 0);
		else
			DoSave (FileName, ".bin", NULL, 0, 0);
		if (!quiet) {
			if (Output89)  printf ("Linked for TI-89\n");
			if (Output92p) printf ("Linked for TI-92 Plus\n");
		}
	} else {
		chksum = LOBYTE (size) + HIBYTE (size);
		unsigned long i;
		for (i=0; i<size; i++) chksum += (unsigned short)dat[i];
		if (Output89)  DoSave (FileName, ".89z", strncpy (Header, "**TI89**", 8), HeaderSize, chksum);
		if (Output92p) DoSave (FileName, ".9xz", strncpy (Header, "**TI92P*", 8), HeaderSize, chksum);
	}
	return TI68kERR_OK;
}

int main (int argc, char **argv)
{
	bool help, outputbin=false, quiet=false, ver=false;
	int err = 0; char *ObjFile=NULL, *OutputName=NULL;
	TI68kType Type = TI68k_none;
	for (help=!--argc; argc && !err; argc--) {
		char *p = *++argv;
		if (*p++ == '-') {
			do switch (*p++) {
				case 'o':
					if (!OutputName) {
						OutputName = *++argv;
						if (--argc) break;
					}
				default: err = -1; break;
				case '?':
				case 'h': help = true; break;
				case 'O': outputbin = true; break;
				case 'q': quiet = true; break;
				case 'v': ver = true; break;
				case 'x': Type = TI68k_ext; break;
				case '-':
					if (!strcmp (p, "help")) help = true;
					else if (!strcmp (p, "version")) ver = true;
					else if (!strcmp (p, "quiet")) quiet = true;
					else if (!strcmp (p, "outputbin")) outputbin = true;
					else if (!strcmp (p, "output") && !OutputName && --argc) OutputName = *++argv;
					else if (!strcmp (p, "ext")) Type = TI68k_ext;
					else err = -1;
				case 0:
					p = NULL;
			}	while (p);
		} else if (ObjFile) err = -1;
		else ObjFile = *argv;
	}

	if (err) {
		fputs ("Invalid command line.\n", stderr);
		help = true;
	}	else if (ver) puts (
		"Obj2Ti 1.01f       by Julien Muchembled <julien.muchembled@netcourrier.com>\n"
		"Modified by:       Romain Lievin <rlievin@mail.com>\n"
		"                   Sebastian Reichelt <Sebastian@tigcc.ticalc.org>\n"
		"                   Kevin Kofler <Kevin@tigcc.ticalc.org>\n"
		"                   and Paul Froissart\n"
	);

	if (help) puts (
			"Usage: obj2ti [options] file\n"
			"Options:\n"
			" -h --help\tDisplay this information\n"
			" -o --output\tFollowed by project name\n"
			" -O --outputbin\tOutput a binary file, not a calc file\n"
			" -q --quiet\tLink without displaying extra verbose information\n"
			" -v --version\tDisplay program version number\n"
			" -x --ext\tOutput a .ext object suitable for GTC\n"
		);
	else if (ObjFile) { 
		TI68k *ti = new TI68k (szErr, Type);
		err = ti->Read (ObjFile);
		if (!err) {
			char *ext = strrchr (OutputName2 = ObjFile, '.');
			if (ext && (!strcmp(ext,".o") || !strcmp(ext,".O")))
				*ext = 0;
			err = ti->Make ();
			if (!err) {
				if (!OutputName) {
					char *ext = strrchr (OutputName = ObjFile, '.');
					if (ext) if (!_stricmp (ext, ".o")) *ext = 0;
				}	err = ti->Save (OutputName, outputbin, quiet);
				if (!err && !quiet) printf ("Linked %s - Size: %lu bytes\n", OutputName, ti->GetExeSize ());
		}	}
		if (err) {
			if (err == TI68kERR_TooLarge)
				fprintf (stderr, "Error: Size of %lu bytes exceeds maximum by %lu.\n", ti->GetExeSize (), ti->GetExeSize () - MAX_VAR_SIZE);
			else if (szErr[err-1])
				fprintf (stderr, "%s.\n", szErr[err-1]);
		}
		delete ti;
	}
	return err;
}
