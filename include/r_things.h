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
// Revision 1.1  2002/11/16 14:18:27  hurdler
// Initial revision
//
// Revision 1.8  2002/09/25 15:17:42  vberghol
// Intermission fixed?
//
// Revision 1.7  2002/09/05 14:12:18  vberghol
// network code partly bypassed
//
// Revision 1.6  2002/08/19 18:06:43  vberghol
// renderer somewhat fixed
//
// Revision 1.5  2002/07/13 17:56:59  vberghol
// *** empty log message ***
//
// Revision 1.4  2002/07/12 19:21:41  vberghol
// hop
//
// Revision 1.3  2002/07/01 21:00:55  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:29  vberghol
// Version 133 Experimental!
//
// Revision 1.10  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
// .
//
// Revision 1.9  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.8  2001/06/16 08:07:55  bpereira
// no message
//
// Revision 1.7  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.6  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.5  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.4  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/30 10:30:10  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------


#ifndef r_things_h
#define r_things_h 1

struct post_t;
typedef post_t column_t;

// number of sprite lumps for spritewidth,offset,topoffset lookup tables
// Fab: this is a hack : should allocate the lookup tables per sprite
#define     MAXSPRITELUMPS     4096

#define MAXVISSPRITES   256 // added 2-2-98 was 128

// Constant arrays used for psprite clipping
//  and initializing clipping.
extern short            negonearray[MAXVIDWIDTH];
extern short            screenheightarray[MAXVIDWIDTH];

// vars for R_DrawMaskedColumn
extern short*           mfloorclip;
extern short*           mceilingclip;
extern fixed_t          spryscale;
extern fixed_t          sprtopscreen;
extern fixed_t          sprbotscreen;
extern fixed_t          windowtop;
extern fixed_t          windowbottom;

extern fixed_t          pspritescale;
extern fixed_t          pspriteiscale;
extern fixed_t          pspriteyscale;  //added:02-02-98:for aspect ratio

extern const int PSpriteSY[];

void R_DrawMaskedColumn(column_t* column);

void R_SortVisSprites();

//faB: find sprites in wadfile, replace existing, add new ones
//     (only sprites from namelist are added or replaced)
void R_AddSpriteDefs(char** namelist, int wadnum);

//SoM: 6/5/2000: Light sprites correctly!
void R_AddSprites(sector_t* sec, int lightlevel);
void R_AddPSprites();
//void R_DrawSprite(vissprite_t* spr);
void R_InitSprites(char** namelist);
void R_ClearSprites();
void R_DrawSprites();  //draw all vissprites
//void R_DrawMasked();

void R_ClipVisSprite(vissprite_t *vis, int xl, int xh);


void R_InitDrawNodes();

#endif