/* Included at the beginning of library source files, AFTER all other #include
lines. Sets up helpful macros and services used in my source code. Since this
is only "active" in my source code, I don't have to worry about polluting the
global namespace with unprefixed names. */

// File_Extractor 1.0.0
#ifndef BLARGG_SOURCE_H
#define BLARGG_SOURCE_H

#ifndef BLARGG_COMMON_H // optimization only
	#include "blargg_common.h"
#endif
#include "blargg_errors.h"

#include <string.h> /* memcpy(), memset(), memmove() */
#include <stddef.h> /* offsetof() */

/* The following four macros are for debugging only. Some or all might be
defined to do nothing, depending on the circumstances. Described is what
happens when a particular macro is defined to do something. When defined to
do nothing, the macros do NOT evaluate their argument(s). */

/* If expr is false, prints file and line number, then aborts program. Meant
for checking internal state and consistency. A failed assertion indicates a bug
in MY code.

void assert( bool expr ); */
#include <assert.h>

/* If expr is false, prints file and line number, then aborts program. Meant
for checking caller-supplied parameters and operations that are outside the
control of the module. A failed requirement probably indicates a bug in YOUR
code.

void require( bool expr ); */
#undef  require
#define require( expr ) assert( expr )

/* Like printf() except output goes to debugging console/file.

void dprintf( const char format [], ... ); */
static inline void blargg_dprintf_( const char [], ... ) { }
#undef  dprintf
#define dprintf(...) ((void)0)

/* If expr is false, prints file and line number to debug console/log, then
continues execution normally. Meant for flagging potential problems or things
that should be looked into, but that aren't serious problems.

void check( bool expr ); */
#undef  check
#define check( expr ) ((void) 0)

/* If expr yields non-NULL error string, returns it from current function,
otherwise continues normally. */
#undef  RETURN_ERR
#define RETURN_ERR( expr ) \
	do {\
		blargg_err_t blargg_return_err_ = (expr);\
		if ( blargg_return_err_ )\
			return blargg_return_err_;\
	} while ( 0 )

/* If ptr is NULL, returns out-of-memory error, otherwise continues normally. */
#undef  CHECK_ALLOC
#define CHECK_ALLOC( ptr ) \
	do {\
		if ( !(ptr) )\
			return blargg_err_memory;\
	} while ( 0 )

/* The usual min/max functions for built-in types. */

template<typename T> T min( T x, T y ) { return x < y ? x : y; }
template<typename T> T max( T x, T y ) { return x > y ? x : y; }

#define BLARGG_DEF_MIN_MAX( type )

BLARGG_DEF_MIN_MAX( int )
BLARGG_DEF_MIN_MAX( unsigned )
BLARGG_DEF_MIN_MAX( long )
BLARGG_DEF_MIN_MAX( unsigned long )
BLARGG_DEF_MIN_MAX( float )
BLARGG_DEF_MIN_MAX( double )
#if __WORDSIZE != 64
BLARGG_DEF_MIN_MAX( BOOST::uint64_t )
#endif

// typedef unsigned char byte;
typedef unsigned char blargg_byte;
#undef  byte
#define byte blargg_byte

#ifndef BLARGG_EXPORT
	#if defined (_WIN32) && BLARGG_BUILD_DLL
		#define BLARGG_EXPORT __declspec(dllexport)
	#elif defined (__GNUC__)
		// can always set visibility, even when not building DLL
		#define BLARGG_EXPORT __attribute__ ((visibility ("default")))
	#else
		#define BLARGG_EXPORT
	#endif
#endif

#if BLARGG_LEGACY
	#define BLARGG_CHECK_ALLOC CHECK_ALLOC
	#define BLARGG_RETURN_ERR  RETURN_ERR
#endif

// Called after failed operation when overall operation may still complete OK.
// Only used by unit testing framework.
#undef ACK_FAILURE
#define ACK_FAILURE() ((void)0)

/* BLARGG_SOURCE_BEGIN: If defined, #included, allowing redefition of dprintf etc.
and check */
#ifdef BLARGG_SOURCE_BEGIN
	#include BLARGG_SOURCE_BEGIN
#endif

#endif
