#ifndef NDEBUG
#define assert(p) ((p)?(void)0:__assertion_failed(#p,__FILE__,__LINE__))
#else
#define assert(p) ((void)0)
#endif
