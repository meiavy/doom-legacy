// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log$
// Revision 1.1  2002/11/16 14:18:22  hurdler
// Initial revision
//
// Revision 1.3  2002/07/01 21:00:45  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:22  vberghol
// Version 133 Experimental!
//
// Revision 1.8  2001/05/16 22:33:34  bock
// Initial FreeBSD support.
//
// Revision 1.7  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.6  2001/02/24 13:35:19  bpereira
// no message
//
// Revision 1.5  2000/11/02 19:49:35  bpereira
// no message
//
// Revision 1.4  2000/10/21 08:43:28  bpereira
// no message
//
// Revision 1.3  2000/08/10 14:53:57  ydario
// OS/2 port
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      doom games standard types
//      Simple basic typedefs, isolated here to make it easier
//      separating modules.
//-----------------------------------------------------------------------------


#ifndef doomtype_h
#define doomtype_h 1

#ifdef __WIN32__
# include <windows.h>
#endif

#if !defined(_OS2EMX_H) && !defined(__WIN32__) 
// VB: no more redefinition warnings (defined in winnt.h)
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif // _OS2EMX_H && __WIN32__

#ifdef __WIN32__
# define INT64  __int64
#else
# define INT64  long long
#endif

#ifdef __APPLE_CC__
# define __MACOS__
//VB: #define DIRECTFULLSCREEN
# define DEBUG_LOG
//VB: #define HWRENDER
#endif

#if defined(__MSC__) || defined(__OS2__)
// Microsoft VisualC++
# define strncasecmp             strnicmp
# define strcasecmp              stricmp
# define inline                  __inline
#else
# ifdef __WATCOMC__
#  include <dos.h>
#  include <sys\types.h>
#  include <direct.h>
#  include <malloc.h>
#  define strncasecmp             strnicmp
#  define strcasecmp              strcmpi
# endif // __WATCOMC__
#endif // __MSC__

// added for Linux 19990220 by Kin
#if defined(LINUX) // standard library differences
# define stricmp(x,y) strcasecmp(x,y)
# define strnicmp(x,y,n) strncasecmp(x,y,n)
# define lstrlen(x) strlen(x)
#endif

#ifdef __APPLE_CC__                //skip all boolean/Boolean crap
# define true 1
# define false 0
# define min(x,y) ( ((x)<(y)) ? (x) : (y) )
# define max(x,y) ( ((x)>(y)) ? (x) : (y) )
# define lstrlen(x) strlen(x)

# define stricmp strcmp
# define strnicmp strncmp

# define __BYTEBOOL__
typedef unsigned char byte;
//# define boolean int

# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif //__MACOS__


#ifndef __BYTEBOOL__
# define __BYTEBOOL__
// from now on, we use always C++ type bool as boolean
typedef unsigned char byte;
//# ifdef __WIN32__
//#  define false   FALSE           // use windows types
//#  define true    TRUE
//#  define boolean BOOL
//# else
//typedef enum {false, true} boolean;
//# endif
    //#endif // __cplusplus
#endif // __BYTEBOOL__

typedef ULONG tic_t;
typedef unsigned int angle_t;

union FColorRGBA {
  ULONG rgba;
  struct {
    byte  red;
    byte  green;
    byte  blue;
    byte  alpha;
  } s;
};

typedef union FColorRGBA RGBA_t;

// Predefined with some OS.
#ifndef __WIN32__
# ifndef __MACOS__
#  ifndef FREEBSD
#   include <values.h>
#  else
#   include <limits.h>
#  endif
# endif
#endif

#ifndef MAXCHAR
# define MAXCHAR   ((char)0x7f)
#endif
#ifndef MAXSHORT
# define MAXSHORT  ((short)0x7fff)
#endif
#ifndef MAXINT
# define MAXINT    ((int)0x7fffffff)
#endif
#ifndef MAXLONG
# define MAXLONG   ((long)0x7fffffff)
#endif

#ifndef MINCHAR
# define MINCHAR   ((char)0x80)
#endif
#ifndef MINSHORT
# define MINSHORT  ((short)0x8000)
#endif
#ifndef MININT
# define MININT    ((int)0x80000000)
#endif
#ifndef MINLONG
# define MINLONG   ((long)0x80000000)
#endif

#endif  //__DOOMTYPE__
