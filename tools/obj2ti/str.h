// Written by Julien Muchembled.
// Fixed by Romain Lievin for Linux.
// Copyright (c) 2001. All rights reserved.

#include <string.h>

#if !defined(WIN32) && !defined(__WIN32__)
# if defined(__UNIX__) || defined(__LINUX__)
#  define _stricmp(s1, s2) (strcasecmp ((s1), (s2)))
# else
int _stricmp (const char *dst, const char *src)
{
	int f,l;

	do {
		f = (unsigned char)*dst++;
		if (f >= 'A' && f <= 'Z') f -= 'A' - 'a';
		l = (unsigned char)*src++;
		if (l >= 'A' && l <= 'Z') l -= 'A' - 'a';
	} while (f && (f == l));
	return f - l;
}
# endif
#endif

#if !defined(WIN32) && !defined(__WIN32__)
char *_strlwr (char *string)
{
	unsigned char *dst = NULL; char * cp;

	for (cp=string; *cp; ++cp)
		if ('A' <= *cp && *cp <= 'Z')
			*cp += 'a' - 'A';
	return string;
}
#endif
