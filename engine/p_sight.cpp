// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.8  2005/03/22 17:02:19  smite-meister
// fix
//
// Revision 1.7  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.6  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.5  2004/07/25 20:19:21  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.4  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.3  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.2  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:03  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief LineOfSight/Visibility checks, uses REJECT and BSP

#include "doomdef.h"

#include "g_actor.h"
#include "g_map.h"
#include "p_maputl.h"
#include "r_defs.h"


//
// P_CheckSight
//
fixed_t         sightzstart;            // eye z of looker
fixed_t         topslope;
fixed_t         bottomslope;            // slopes to top and bottom of target

static divline_t strace;                 // from t1 to t2
static fixed_t   t2x, t2y;
static int       sightcounts[2];


/// Returns side 0 (front), 1 (back), or 2 (on).
static int P_DivlineSide(fixed_t x, fixed_t y, divline_t* node)
{
  fixed_t dx, dy, left, right;

  if (!node->dx)
    {
      if (x==node->x)
        return 2;

      if (x <= node->x)
        return node->dy > 0;

      return node->dy < 0;
    }

  if (!node->dy)
    {
      if (x==node->y)
        return 2;

      if (y <= node->y)
        return node->dx < 0;

      return node->dx > 0;
    }

  dx = (x - node->x);
  dy = (y - node->y);

  left =  (node->dy>>FRACBITS) * (dx>>FRACBITS);
  right = (dy>>FRACBITS) * (node->dx>>FRACBITS);

  if (right < left)
    return 0;       // front side

  if (left == right)
    return 2;
  return 1;           // back side
}


// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings and addlines traversers.
static fixed_t P_InterceptVector2(divline_t *v2, divline_t *v1)
{
  fixed_t den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

  if (den == 0)
    return 0;
  //  I_Error ("P_InterceptVector: parallel");

  fixed_t num = FixedMul((v1->x - v2->x) >> 8, v1->dy) + FixedMul((v2->y - v1->y) >> 8, v1->dx);
  fixed_t frac = FixedDiv (num , den);

  return frac;
}


// Returns true if strace crosses the given subsector successfully.
bool Map::CrossSubsector(int num)
{
#ifdef RANGECHECK
  if (num>=numsubsectors)
    I_Error ("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  subsector_t *sub = &subsectors[num];

  // check lines
  int count = sub->numlines;
  seg_t *seg = &segs[sub->firstline];

  for ( ; count ; seg++, count--)
    {
      line_t *line = seg->linedef;

      // allready checked other side?
      if (line->validcount == validcount)
        continue;

      line->validcount = validcount;

      vertex_t *v1 = line->v1;
      vertex_t *v2 = line->v2;
      int s1 = P_DivlineSide(v1->x, v1->y, &strace);
      int s2 = P_DivlineSide(v2->x, v2->y, &strace);

      // line isn't crossed?
      if (s1 == s2)
        continue;

      divline_t divl;

      divl.x = v1->x;
      divl.y = v1->y;
      divl.dx = v2->x - v1->x;
      divl.dy = v2->y - v1->y;
      s1 = P_DivlineSide(strace.x, strace.y, &divl);
      s2 = P_DivlineSide(t2x, t2y, &divl);

      // line isn't crossed?
      if (s1 == s2)
        continue;

      // stop because it is not two sided anyway
      // might do this after updating validcount?
      if ( !(line->flags & ML_TWOSIDED) )
        return false;

      // crosses a two sided line
      sector_t *front = seg->frontsector;
      sector_t *back = seg->backsector;

      // no wall to block sight with?
      if (front->floorheight == back->floorheight
          && front->ceilingheight == back->ceilingheight)
        continue;

      fixed_t opentop, openbottom;

      // possible occluder
      // because of ceiling height differences
      if (front->ceilingheight < back->ceilingheight)
        opentop = front->ceilingheight;
      else
        opentop = back->ceilingheight;

      // because of ceiling height differences
      if (front->floorheight > back->floorheight)
        openbottom = front->floorheight;
      else
        openbottom = back->floorheight;

      // quick test for totally closed doors
      if (openbottom >= opentop)
        return false;               // stop

      fixed_t frac = P_InterceptVector2(&strace, &divl);      

      if (front->floorheight != back->floorheight)
        {
          fixed_t slope = FixedDiv(openbottom - sightzstart , frac);
          if (slope > bottomslope)
            bottomslope = slope;
        }

      if (front->ceilingheight != back->ceilingheight)
        {
          fixed_t slope = FixedDiv (opentop - sightzstart , frac);
          if (slope < topslope)
            topslope = slope;
        }

      if (topslope <= bottomslope)
        return false;               // stop
    }
  // passed the subsector ok
  return true;
}



// Returns true if strace crosses the given node successfully.
bool Map::CrossBSPNode(int bspnum)
{
  if (bspnum & NF_SUBSECTOR)
    {
      if (bspnum == -1)
        return CrossSubsector(0);
      else
        return CrossSubsector(bspnum & (~NF_SUBSECTOR));
    }

  node_t *bsp = &nodes[bspnum];

  // decide which side the start point is on
  int side = P_DivlineSide(strace.x, strace.y, (divline_t *)bsp);
  if (side == 2)
    side = 0;       // an "on" should cross both sides

  // cross the starting side
  if (!CrossBSPNode(bsp->children[side]))
    return false;

  // the partition plane is crossed here
  if (side == P_DivlineSide(t2x, t2y,(divline_t *)bsp))
    {
      // the line doesn't touch the other side
      return true;
    }

  // cross the ending side
  return CrossBSPNode(bsp->children[side^1]);
}


// Returns true if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
bool Map::CheckSight(Actor *t1, Actor *t2)
{
  if (!t1 || !t2)
    return false;

  // First check for trivial rejection.

  // Determine subsector entries in REJECT table.
  int s1 = (t1->subsector->sector - sectors);
  int s2 = (t2->subsector->sector - sectors);
  int pnum = s1*numsectors + s2;
  int bytenum = pnum>>3;
  int bitnum = 1 << (pnum&7);

  // Check in REJECT table.
  if (rejectmatrix[bytenum]&bitnum)
    {
      sightcounts[0]++;

      // can't possibly be connected
      return false;
    }
  /*  BP: it seam that it don't work :( TODO: fix it
      if (gamemode == heretic )
      {
      //
      // check precisely
      //
      sightzstart = t1->z + t1->height - (t1->height>>2);
      topslope = (t2->z+t2->height) - sightzstart;
      bottomslope = (t2->z) - sightzstart;

      return P_SightPathTraverse ( t1->x, t1->y, t2->x, t2->y );
      }
  */
  // An unobstructed LOS is possible.
  // Now look from eyes of t1 to any part of t2.
  sightcounts[1]++;

  validcount++;

  sightzstart = t1->z + t1->height - (t1->height>>2);
  topslope = (t2->z+t2->height) - sightzstart;
  bottomslope = (t2->z) - sightzstart;

  strace.x = t1->x;
  strace.y = t1->y;
  t2x = t2->x;
  t2y = t2->y;
  strace.dx = t2->x - t1->x;
  strace.dy = t2->y - t1->y;

  // the head node is the last node output
  return CrossBSPNode(numnodes-1);
}
