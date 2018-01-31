typedef struct {
    char f_magic[2];
    char f_nscns[2];  // # of sections
    char f_timdat[4]; // timestamp
    char f_symptr[4]; // ptr to symtab
    char f_nsyms[4];  // # of symtab entries
    char f_opthdr[2]; // optional header size
    char f_flags[2];
} FILHDR;

typedef struct {
    char s_name[8];
    char s_paddr[4];
    char s_vaddr[4];
    char s_size[4];
    char s_scnptr[4];
    char s_relptr[4];
    char s_lnnoptr[4];
    char s_nreloc[2];
    char s_nlnno[2];
    char s_flags[4];
} SCNHDR;

typedef struct {
    union {
	char e_name[8];
	struct {
	    char e_zeroes[4];
	    char e_offset[4];
	} e;
    } e;
    char e_value[4];
    char e_scnum[2];
    char e_type[2];
    char e_sclass[1];
    char e_numaux[1];
} SYMENT;

typedef struct {
    char r_vaddr[4];
    char r_symndx[4];
    char r_type[2];
} RELOC;
