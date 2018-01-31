// ugly arbitrary limits
#ifndef LIMITS_H_
#define LIMITS_H_

#define MAX_OPEN_PCH 48

/* each entry will consume 8 bytes */
#define MAX_CATALOG_ENTRIES 3000

#ifdef PC
#define MAX_PCHDATA_SIZE 20000
#endif

#endif
