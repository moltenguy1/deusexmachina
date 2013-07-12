#ifndef N_TYPES_H
#define N_TYPES_H

#include <stdio.h> //???why error if comment?
#include <stdlib.h> // malloc, free
#include <new> // malloc, free
#include "StdCfg.h"

//---------------------------------------------------------------------
//  Debug macros
//---------------------------------------------------------------------
#ifdef DEM_NO_ASSERT
	#define n_verify(exp) (exp)
	#define n_assert(exp)
	#define n_assert2(exp, msg)
	#define n_assert_dbg(exp)
	#define n_assert2_dbg(exp, msg)
#else
	#define n_verify(exp)				{ if (!(exp)) n_barf(#exp, __FILE__, __LINE__); }
	#define n_assert(exp)				{ if (!(exp)) n_barf(#exp, __FILE__, __LINE__); }
	#define n_assert2(exp, msg)			{ if (!(exp)) n_barf2(#exp, msg, __FILE__, __LINE__); }

	#ifdef _DEBUG
		#define n_verify_dbg(exp)		n_verify(exp)
		#define n_assert_dbg(exp)		n_assert(exp)
		#define n_assert2_dbg(exp, msg)	n_assert2(exp, msg)
	#else
		#define n_verify_dbg(exp)		(exp)
		#define n_assert_dbg(exp)
		#define n_assert2_dbg(exp, msg)
	#endif
#endif

//---------------------------------------------------------------------
//  Shortcut typedefs
//---------------------------------------------------------------------
typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef double nTime;

#ifndef NULL
#define NULL (0L)
#endif

enum EClipStatus
{
	Outside,
	Inside,
	Clipped,
	//InvalidClipStatus - Clipped is used instead now
};

#define N_MAXPATH (512)     // maximum length for complete path

#define N_WHITESPACE " \r\n\t"

#if defined(__LINUX__) || defined(__MACOSX__)
#define n_stricmp strcasecmp
#else
#define n_stricmp stricmp
#endif

#ifdef __WIN32__
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#ifdef _MSC_VER
#define va_copy(d, s) d = s
#endif

#ifdef __GNUC__
// Hey! Look! A cute GNU C++ extension!
#define min(a, b)   a <? b
#define max(a, b)   a >? b
#endif

// maps unsigned 8 bits/channel to D3DCOLOR
#define N_ARGB(a, r, g, b) ((uint)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))
#define N_RGBA(r, g, b, a) N_ARGB(a, r, g, b)
#define N_XRGB(r, g, b)   N_ARGB(0xff, r, g, b)
#define N_COLORVALUE(r, g, b, a) N_RGBA((uint)((r)*255.f), (uint)((g)*255.f), (uint)((b)*255.f), (uint)((a)*255.f))

//------------------------------------------------------------------------------
//  public kernel C functions
//------------------------------------------------------------------------------
void __cdecl n_printf(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void __cdecl n_error(const char*, ...)
    __attribute__((noreturn))
    __attribute__((format(printf, 1, 2)));
void __cdecl n_message(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void __cdecl n_dbgout(const char*, ...)
    __attribute__((format(printf, 1, 2)));
void n_sleep(double);
char* n_strdup(const char*);
char* n_strncpy2(char*, const char*, size_t);
bool n_strmatch(const char*, const char*);
void n_strcat(char*, const char*, size_t);

void n_barf(const char*, const char*, int)
    __attribute__((noreturn));
void n_barf2(const char*, const char*, const char*, int)
    __attribute__((noreturn));

//------------------------------------------------------------------------------
//  Nebula memory management and debugging stuff.
//------------------------------------------------------------------------------
extern bool DEM_LogMemory;
struct nMemoryStats
{
	int HighWaterSize;      // max allocated size so far
	int TotalCount;         // total number of allocations
	int TotalSize;          // current allocated size
};

int n_dbgmemdumpleaks();
void n_dbgmeminit();                // initialize memory debugging system
nMemoryStats n_dbgmemgetstats();    // defined in ndbgalloc.cc

#ifdef new
#undef new
#endif

#ifdef delete
#undef delete
#endif

// implemented in ndbgalloc.cc
void* operator new(size_t size);
void* operator new(size_t size, const char* file, int line);
void* operator new(size_t size, void* place, const char* file, int line);
void* operator new[](size_t size);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* p);
void operator delete(void* p, const char* file, int line);
void operator delete(void*, void*, const char* file, int line);
void operator delete[](void* p);
void operator delete[](void* p, const char* file, int line);
void* n_malloc_dbg(size_t size, const char* file, int line);
void* n_calloc_dbg(size_t num, size_t size, const char* file, int line);
void* n_realloc_dbg(void* memblock, size_t size, const char* file, int line);
void n_free_dbg(void* memblock, const char* file, int line);

#if defined(_DEBUG) && defined(__WIN32__)
#define n_new(type) new(__FILE__,__LINE__) type
#define n_placement_new(place, type) new(place, __FILE__,__LINE__) type
#define n_new_array(type,size) new(__FILE__,__LINE__) type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) n_malloc_dbg(size, __FILE__, __LINE__)
#define n_calloc(num, size) n_calloc_dbg(num, size, __FILE__, __LINE__)
#define n_realloc(memblock, size) n_realloc_dbg(memblock, size, __FILE__, __LINE__)
#define n_free(memblock) n_free_dbg(memblock, __FILE__, __LINE__)
#else
#define n_new(type) new type
#define n_placement_new(place, type) new(place) type
#define n_new_array(type,size) new type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
#define n_malloc(size) malloc(size)
#define n_calloc(num, size) calloc(num, size)
#define n_realloc(memblock, size) realloc(memblock, size)
#define n_free(memblock) free(memblock)
#endif

#endif
