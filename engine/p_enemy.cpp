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
// Revision 1.1  2002/11/16 14:17:55  hurdler
// Initial revision
//
// Revision 1.14  2002/09/20 22:41:29  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.13  2002/08/23 09:53:41  vberghol
// fixed Actor:: target/owner/tracer
//
// Revision 1.12  2002/08/21 16:58:31  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.11  2002/08/17 21:21:45  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.10  2002/08/14 17:07:18  vberghol
// p_map.cpp done... 3 to go
//
// Revision 1.9  2002/08/13 19:47:41  vberghol
// p_inter.cpp done
//
// Revision 1.8  2002/08/11 17:16:48  vberghol
// ...
//
// Revision 1.7  2002/08/06 13:14:21  vberghol
// ...
//
// Revision 1.6  2002/08/02 20:14:49  vberghol
// p_enemy.cpp done!
//
// Revision 1.5  2002/07/26 19:23:04  vberghol
// a little something
//
// Revision 1.4  2002/07/23 19:21:40  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:16  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:12  vberghol
// Version 133 Experimental!
//
// Revision 1.12  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.10  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.9  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.3  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Enemy thinking, AI.
//      Action Pointer Functions
//      that are associated with states/frames.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "p_enemy.h"
#include "command.h"
#include "p_spec.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h" // remove...

#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"
#include "p_maputl.h"

#include "hardware/hw3sound.h"



void FastMonster_OnChange(void);

// enable the solid corpses option : still not finished
consvar_t cv_solidcorpse = {"solidcorpse","0",CV_NETVAR | CV_SAVE,CV_OnOff};
consvar_t cv_fastmonsters = {"fastmonsters","0",CV_NETVAR | CV_CALL,CV_OnOff,FastMonster_OnChange};


typedef enum
{
  DI_EAST,
  DI_NORTHEAST,
  DI_NORTH,
  DI_NORTHWEST,
  DI_WEST,
  DI_SOUTHWEST,
  DI_SOUTH,
  DI_SOUTHEAST,
  DI_NODIR,
  NUMDIRS

} dirtype_t;


//
// P_NewChaseDir related LUT.
//
static dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

static dirtype_t diags[] =
{
  DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};


void FastMonster_OnChange(void)
{
  static bool fast=false;
  static const struct {
    mobjtype_t type;
    int speed[2];
  } MonsterMissileInfo[] =
    {
      // doom
      { MT_BRUISERSHOT, {15, 20}},
      { MT_HEADSHOT,    {10, 20}},
      { MT_TROOPSHOT,   {10, 20}},
        
      // heretic
      { MT_IMPBALL,     {10, 20}},
      { MT_MUMMYFX1,    { 9, 18}},
      { MT_KNIGHTAXE,   { 9, 18}},
      { MT_REDAXE,      { 9, 18}},
      { MT_BEASTBALL,   {12, 20}},
      { MT_WIZFX1,      {18, 24}},
      { MT_SNAKEPRO_A,  {14, 20}},
      { MT_SNAKEPRO_B,  {14, 20}},
      { MT_HEADFX1,     {13, 20}},
      { MT_HEADFX3,     {10, 18}},
      { MT_MNTRFX1,     {20, 26}},
      { MT_MNTRFX2,     {14, 20}},
      { MT_SRCRFX1,     {20, 28}},
      { MT_SOR2FX1,     {20, 28}},
      
      { mobjtype_t(-1), {-1, -1} } // Terminator
    };

  int i;
  if (cv_fastmonsters.value && !fast)
    {
      for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	states[i].tics >>= 1;
      fast=true;
    }
  else
    if(!cv_fastmonsters.value && fast)
      {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	  states[i].tics <<= 1;
        fast=false;
      }

  for(i = 0; MonsterMissileInfo[i].type != -1; i++)
    {
      mobjinfo[MonsterMissileInfo[i].type].speed
	= MonsterMissileInfo[i].speed[cv_fastmonsters.value]<<FRACBITS;
    }
}

//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

static Actor   *soundtarget;

static void P_RecursiveSound(const Map *m, sector_t *sec, int soundblocks)
{
  int         i;
  line_t *check;
  sector_t *other;

  // wake up all monsters in this sector
  if (sec->validcount == validcount
      && sec->soundtraversed <= soundblocks+1)
    {
      return;         // already flooded
    }

  sec->validcount = validcount;
  sec->soundtraversed = soundblocks+1;
  sec->soundtarget = soundtarget;

  for (i=0 ;i<sec->linecount ; i++)
    {
      check = sec->lines[i];
      if (! (check->flags & ML_TWOSIDED) )
	continue;

      P_LineOpening (check);

      if (openrange <= 0)
	continue;   // closed door

      if ( m->sides[ check->sidenum[0] ].sector == sec)
	other = m->sides[ check->sidenum[1] ].sector;
      else
	other = m->sides[ check->sidenum[0] ].sector;

      if (check->flags & ML_SOUNDBLOCK)
        {
	  if (!soundblocks)
	    P_RecursiveSound (m, other, 1);
        }
      else
	P_RecursiveSound (m, other, soundblocks);
    }
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void P_NoiseAlert(Actor *target, Actor *emitter)
{
  soundtarget = target;
  validcount++;
  P_RecursiveSound(emitter->mp, emitter->subsector->sector, 0);
}




//
// was P_CheckMeleeRange
//
bool Actor::CheckMeleeRange()
{
  Actor *pl = target;
  fixed_t     dist;

  if (pl == NULL)
    return false;

  dist = P_AproxDistance(pl->x - x, pl->y - y);

  if (dist >= MELEERANGE-20*FRACUNIT+pl->info->radius)
    return false;

  //added:19-03-98: check height now, so that damn imps cant attack
  //                you if you stand on a higher ledge.
  if (game.demoversion>111 &&
      ((pl->z > z+height) || (z > pl->z + pl->height) ))
    return false;

  if (!mp->CheckSight(this, target))
    return false;

  return true;
}

//
// P_CheckMissileRange
//
static bool P_CheckMissileRange(Actor *actor)
{
  fixed_t     dist;

  if (!actor->mp->CheckSight(actor, actor->target) )
    return false;

  if ( actor->flags & MF_JUSTHIT )
    {
      // the target just hit the enemy,
      // so fight back!
      actor->flags &= ~MF_JUSTHIT;
      return true;
    }

  if (actor->reactiontime)
    return false;   // do not attack yet

  // OPTIMIZE: get this from a global checksight
  dist = P_AproxDistance ( actor->x-actor->target->x,
			   actor->y-actor->target->y) - 64*FRACUNIT;

  if (!actor->info->meleestate)
    dist -= 128*FRACUNIT;   // no melee attack, so fire more

  dist >>= 16;

  if (actor->type == MT_VILE)
    {
      if (dist > 14*64)
	return false;       // too far away
    }

  if (actor->type == MT_UNDEAD)
    {
      if (dist < 196)
	return false;       // close for fist attack
      dist >>= 1;
    }


  if (actor->type == MT_CYBORG
      || actor->type == MT_SPIDER
      || actor->type == MT_SKULL
      || actor->type == MT_IMP) // heretic monster
    {
      dist >>= 1;
    }

  if (dist > 200)
    dist = 200;

  if (actor->type == MT_CYBORG && dist > 160)
    dist = 160;

  if (P_Random () < dist)
    return false;

  return true;
}


//
// P_Move
// Move non-player  in the current direction,
// returns false if the move is blocked.
//
static const fixed_t xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
static const fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};

static bool P_Move(Actor *actor)
{
  // movement function communication variables
  extern int  numspechit;
  extern line_t **spechit;

  fixed_t     tryx;
  fixed_t     tryy;

  line_t *ld;

  bool     good;

  if (actor->movedir == DI_NODIR)
    return false;

#ifdef PARANOIA
  if ((unsigned)actor->movedir >= 8)
    I_Error ("Weird actor->movedir!");
#endif

  tryx = actor->x + actor->info->speed*xspeed[actor->movedir];
  tryy = actor->y + actor->info->speed*yspeed[actor->movedir];

  if (!actor->TryMove (tryx, tryy, false))
    {
      // open any specials
      if (actor->flags & MF_FLOAT && floatok)
        {
	  // must adjust height
	  if (actor->z < tmfloorz)
	    actor->z += FLOATSPEED;
	  else
	    actor->z -= FLOATSPEED;

	  actor->flags |= MF_INFLOAT;
	  return true;
        }

      if (!numspechit)
	return false;

      actor->movedir = DI_NODIR;
      good = false;
      while (numspechit--)
        {
	  //ld = actor->mp->lines + spechit[numspechit];
	  ld = spechit[numspechit];
	  // if the special is not a door
	  // that can be opened,
	  // return false
	  if (actor->mp->UseSpecialLine(actor, ld, 0))
	    good = true;
        }
      return good;
    }
  else
    {
      actor->flags &= ~MF_INFLOAT;
    }


  if (! (actor->flags & MF_FLOAT) )
    {
      if(actor->z > actor->floorz)
	actor->HitFloor();
      actor->z = actor->floorz;
    }
  return true;
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
static bool P_TryWalk(Actor *actor)
{
  if (!P_Move (actor))
    {
      return false;
    }
  actor->movecount = P_Random()&15;
  return true;
}




static void P_NewChaseDir(Actor *actor)
{
  fixed_t     deltax;
  fixed_t     deltay;

  dirtype_t   d[3];

  int         tdir;
  dirtype_t   olddir;

  dirtype_t   turnaround;

  if (!actor->target)
    I_Error ("P_NewChaseDir: called with no target");

  olddir = dirtype_t(actor->movedir);
  turnaround=opposite[olddir];

  deltax = actor->target->x - actor->x;
  deltay = actor->target->y - actor->y;

  if (deltax>10*FRACUNIT)
    d[1]= DI_EAST;
  else if (deltax<-10*FRACUNIT)
    d[1]= DI_WEST;
  else
    d[1]= DI_NODIR;

  if (deltay<-10*FRACUNIT)
    d[2]= DI_SOUTH;
  else if (deltay>10*FRACUNIT)
    d[2]= DI_NORTH;
  else
    d[2]= DI_NODIR;

  // try direct route
  if (   d[1] != DI_NODIR
	 && d[2] != DI_NODIR)
    {
      actor->movedir = diags[((deltay<0)<<1)+(deltax>0)];
      if (actor->movedir != turnaround && P_TryWalk(actor))
	return;
    }

  // try other directions
  if (P_Random() > 200
      ||  abs(deltay)>abs(deltax))
    {
      tdir=d[1];
      d[1]=d[2];
      d[2]=dirtype_t(tdir);
    }

  if (d[1]==turnaround)
    d[1]=DI_NODIR;
  if (d[2]==turnaround)
    d[2]=DI_NODIR;

  if (d[1]!=DI_NODIR)
    {
      actor->movedir = d[1];
      if (P_TryWalk(actor))
        {
	  // either moved forward or attacked
	  return;
        }
    }

  if (d[2]!=DI_NODIR)
    {
      actor->movedir =d[2];

      if (P_TryWalk(actor))
	return;
    }

  // there is no direct path to the player,
  // so pick another direction.
  if (olddir!=DI_NODIR)
    {
      actor->movedir =olddir;

      if (P_TryWalk(actor))
	return;
    }

  // randomly determine direction of search
  if (P_Random()&1)
    {
      for ( tdir=DI_EAST;
	    tdir<=DI_SOUTHEAST;
	    tdir++ )
        {
	  if (tdir!=turnaround)
            {
	      actor->movedir =tdir;

	      if ( P_TryWalk(actor) )
		return;
            }
        }
    }
  else
    {
      for ( tdir=DI_SOUTHEAST;
	    tdir >= DI_EAST;
	    tdir-- )
        {
	  if (tdir!=turnaround)
            {
	      actor->movedir =tdir;

	      if ( P_TryWalk(actor) )
		return;
            }
        }
    }

  if (turnaround !=  DI_NODIR)
    {
      actor->movedir =turnaround;
      if ( P_TryWalk(actor) )
	return;
    }

  actor->movedir = DI_NODIR;  // can not move
}



//
// was P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
bool Actor::LookForPlayers(bool allaround)
{
  int         c, stop;
  PlayerPawn *p;
  angle_t     an;
  fixed_t     dist;

  // FIXME removed until LookForMonsters is fixed
  //if (!game.multiplayer && consoleplayer->pawn->health <= 0 && game.mode == heretic)
  //  // Single player game and player is dead, look for monsters
  //  return LookForMonsters();

  //sector_t *sector = subsector->sector;
  // FIXME bug why is n == 0 ?
  int n = mp->players.size();
  if (n == 0)
    return false;
  // BP: first time init, this allow minimum lastlook changes
  if (lastlook < 0 && game.demoversion >= 129)
    lastlook = P_Random () % n;

  c = 0;
  stop = (lastlook - 1) % n;

  for ( ; ; lastlook = lastlook+1 )
    {
      if (lastlook >= n) lastlook = 0;

      // done looking
      if (lastlook == stop)
	return false;

      //if (!playeringame[lastlook]) continue;

      if (c++ == 2)
	return false;

      p = mp->players[lastlook]->pawn;

      if (p->health <= 0)
	continue;           // dead

      if (!mp->CheckSight(this, p))
	continue;           // out of sight

      if (!allaround)
        {
	  an = R_PointToAngle2(x, y, p->x, p->y) - angle;

	  if (an > ANG90 && an < ANG270)
            {
	      dist = P_AproxDistance(p->x - x, p->y - y);
	      // if real close, react anyway
	      if (dist > MELEERANGE)
		continue;   // behind back
            }
        }

      if (game.mode == heretic && p->flags & MF_SHADOW)
        { // Player is invisible
	  if ((P_AproxDistance(p->x - x, p->y - y) > 2*MELEERANGE)
	      && P_AproxDistance(p->px, p->py) < 5*FRACUNIT)
            { // Player is sneaking - can't detect
	      return false;
            }
	  if (P_Random() < 225)
            { // Player isn't sneaking, but still didn't detect
	      return false ;
            }
        }

      target = p;
      return true;
    }

  return false;
}

// was P_LookForMonsters
/* FIXME
#define MONS_LOOK_RANGE (20*64*FRACUNIT)
#define MONS_LOOK_LIMIT 64

bool Actor::LookForMonsters()
{
  Actor *mo;
  Thinker *t, *tc;

  if (!mp->CheckSight(game.players[0]->pawn, this))
    // Player can't see monster
    return false;

  int count = 0;
  tc = &mp->thinkercap;
  for (t = tc->next; t != tc; t = t->next)
    {
      if (t->Type() != Thinker::tt_actor)
	// Not a mobj thinker
	continue;
	
      mo = (Actor *)t;
      if (!(mo->flags & MF_COUNTKILL) || (mo == this) || (mo->health <= 0))
	{ // Not a valid monster
	  continue;
	}
      if (P_AproxDistance(x - mo->x, y - mo->y) > MONS_LOOK_RANGE)
	{ // Out of range
	  continue;
	}
      if (P_Random() < 16)
	{ // Skip
	  continue;
	}
      if (count++ > MONS_LOOK_LIMIT)
	{ // Stop searching
	  return false;
	}
      if (!mp->CheckSight(this, mo))
	{ // Out of sight
	  continue;
	}
      // Found a target monster
      target = mo;
      return true;
    }
  return false;
}
*/
//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look(Actor *actor)
{
  actor->threshold = 0;       // any shot will wake up
  Actor *targ = actor->subsector->sector->soundtarget;

  if (targ && (targ->flags & MF_SHOOTABLE))
    {
      actor->target = targ;

      if (actor->flags & MF_AMBUSH)
        {
	  if (actor->mp->CheckSight(actor, actor->target))
	    goto seeyou;
        }
      else
	goto seeyou;
    }


  if (!actor->LookForPlayers(false))
    return;

  // go into chase state
 seeyou:
  if (actor->info->seesound)
    {
      int             sound;

      switch (actor->info->seesound)
        {
	case sfx_posit1:
	case sfx_posit2:
	case sfx_posit3:
	  sound = sfx_posit1+P_Random()%3;
	  break;

	case sfx_bgsit1:
	case sfx_bgsit2:
	  sound = sfx_bgsit1+P_Random()%2;
	  break;

	default:
	  sound = actor->info->seesound;
	  break;
        }

      if (actor->type==MT_SPIDER
	  || actor->type == MT_CYBORG
	  || (actor->flags2 & MF2_BOSS))
        {
	  // full volume
	  S_StartAmbSound(sound);
        }
      else
	S_StartScreamSound(actor, sound);
    }

  actor->SetState(actor->info->seestate);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase(Actor *actor)
{
  int         delta;

  if (actor->reactiontime)
    actor->reactiontime--;

  // modify target threshold
  if (actor->threshold)
    {
      if (game.mode != heretic && (!actor->target
				   || actor->target->health <= 0
				   || (actor->target->flags & MF_CORPSE)))
        {
	  actor->threshold = 0;
        }
      else
	actor->threshold--;
    }

  if (game.mode == heretic  && cv_fastmonsters.value)
    { // Monsters move faster in nightmare mode
      actor->tics -= actor->tics/2;
      if (actor->tics < 3)
	actor->tics = 3;
    }

  // turn towards movement direction if not there yet
  if (actor->movedir < 8)
    {
      actor->angle &= (7<<29);
      delta = actor->angle - (actor->movedir << 29);

      if (delta > 0)
	actor->angle -= ANG90/2;
      else if (delta < 0)
	actor->angle += ANG90/2;
    }

  if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {
      // look for a new target
      if (actor->LookForPlayers(true))
	return;     // got a new target

      actor->SetState(actor->info->spawnstate);
      return;
    }

  // do not attack twice in a row
  if (actor->flags & MF_JUSTATTACKED)
    {
      actor->flags &= ~MF_JUSTATTACKED;
      if (!cv_fastmonsters.value)
	P_NewChaseDir(actor);
      return;
    }

  // check for melee attack
  if (actor->info->meleestate && actor->CheckMeleeRange())
    {
      if (actor->info->attacksound)
	S_StartAttackSound(actor, actor->info->attacksound);

      actor->SetState(actor->info->meleestate);
      return;
    }

  // check for missile attack
  if (actor->info->missilestate)
    {
      if (!cv_fastmonsters.value && actor->movecount)
        {
	  goto nomissile;
        }

      if (!P_CheckMissileRange (actor))
	goto nomissile;

      actor->SetState(actor->info->missilestate);
      actor->flags |= MF_JUSTATTACKED;
      return;
    }

  // ?
 nomissile:
  // possibly choose another target
  if (game.multiplayer && !actor->threshold
      && !actor->mp->CheckSight(actor, actor->target) )
    {
      if (actor->LookForPlayers(true))
	return;     // got a new target
    }

  // chase towards player
  if (--actor->movecount<0
      || !P_Move (actor))
    {
      P_NewChaseDir (actor);
    }

  // make active sound
  if (actor->info->activesound
      && P_Random () < 3)
    {
      if(actor->type == MT_WIZARD && P_Random() < 128)
	S_StartScreamSound(actor, actor->info->seesound);
      else if(actor->type == MT_SORCERER2)
	S_StartAmbSound(actor->info->activesound);
      else
	S_StartScreamSound(actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget(Actor *actor)
{
  if (!actor->target)
    return;

  actor->flags &= ~MF_AMBUSH;

  actor->angle = R_PointToAngle2(actor->x, actor->y,
				 actor->target->x,
				 actor->target->y);

  if (actor->target->flags & MF_SHADOW)
    actor->angle += P_SignedRandom()<<21;
}


//
// A_PosAttack
//
void A_PosAttack(Actor *actor)
{
  int         angle;
  int         damage;
  int         slope;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  angle = actor->angle;
  slope = actor->AimLineAttack(angle, MISSILERANGE);
  S_StartAttackSound(actor, sfx_pistol);
  angle += P_SignedRandom()<<20;
  damage = ((P_Random()%5)+1)*3;
  actor->LineAttack(angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack(Actor *actor)
{
  int         i;
  int         angle;
  int         bangle;
  int         damage;
  int         slope;

  if (!actor->target)
    return;
  S_StartAttackSound(actor, sfx_shotgn);
  A_FaceTarget (actor);
  bangle = actor->angle;
  slope = actor->AimLineAttack(bangle, MISSILERANGE);

  for (i=0 ; i<3 ; i++)
    {
      angle  = (P_SignedRandom()<<20)+bangle;
      damage = ((P_Random()%5)+1)*3;
      actor->LineAttack(angle, MISSILERANGE, slope, damage);
    }
}

void A_CPosAttack(Actor *actor)
{
  int         angle;
  int         bangle;
  int         damage;
  int         slope;

  if (!actor->target)
    return;
  S_StartAttackSound(actor, sfx_shotgn);
  A_FaceTarget (actor);
  bangle = actor->angle;
  slope = actor->AimLineAttack(bangle, MISSILERANGE);

  angle  = (P_SignedRandom()<<20)+bangle;

  damage = ((P_Random()%5)+1)*3;
  actor->LineAttack(angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire(Actor *actor)
{
  // keep firing unless target got out of sight
  A_FaceTarget (actor);

  if (P_Random () < 40)
    return;

  if (!actor->target
      || actor->target->health <= 0
      || actor->target->flags & MF_CORPSE
      || !actor->mp->CheckSight(actor, actor->target) )
    {
      actor->SetState(actor->info->seestate);
    }
}


void A_SpidRefire(Actor *actor)
{
  // keep firing unless target got out of sight
  A_FaceTarget (actor);

  if (P_Random () < 10)
    return;

  if (!actor->target
      || actor->target->health <= 0
      || actor->target->flags & MF_CORPSE
      || !actor->mp->CheckSight(actor, actor->target) )
    {
      actor->SetState(actor->info->seestate);
    }
}

void A_BspiAttack(Actor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget(actor);

  // launch a missile
  actor->SpawnMissile(actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack(Actor *actor)
{
  int damage;

  if (!actor->target)
    return;

  A_FaceTarget(actor);
  if (actor->CheckMeleeRange())
    {
      S_StartAttackSound(actor, sfx_claw);
      damage = (P_Random()%8+1)*3;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_TROOPSHOT);
}


void A_SargAttack(Actor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  if (actor->CheckMeleeRange())
    {
      damage = ((P_Random()%10)+1)*4;
      actor->target->Damage(actor, actor, damage);
    }
}

void A_HeadAttack(Actor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  if (actor->CheckMeleeRange())
    {
      damage = (P_Random()%6+1)*10;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_HEADSHOT);
}

void A_CyberAttack(Actor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);
  actor->SpawnMissile(actor->target, MT_ROCKET);
}


void A_BruisAttack(Actor *actor)
{
  int         damage;

  if (!actor->target)
    return;

  if (actor->CheckMeleeRange())
    {
      S_StartAttackSound(actor, sfx_claw);
      damage = (P_Random()%8+1)*10;
      actor->target->Damage(actor, actor, damage);
      return;
    }

  // launch a missile
  actor->SpawnMissile(actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile(Actor *actor)
{
  Actor  *mo;

  if (!actor->target)
    return;

  A_FaceTarget (actor);
  actor->z += 16*FRACUNIT;    // so missile spawns higher
  mo = actor->SpawnMissile(actor->target, MT_TRACER);
  actor->z -= 16*FRACUNIT;    // back to normal

  if(mo)
    {
      mo->x += mo->px;
      mo->y += mo->py;
      mo->target = actor->target;
    }
}

int     TRACEANGLE = 0xc000000;

// guide a guiding missile
void A_Tracer(Actor *actor)
{
  angle_t     exact;
  fixed_t     dist;
  fixed_t     slope;

  Map *m = actor->mp;

  if (gametic % (4 * NEWTICRATERATIO))
    return;

  // spawn a puff of smoke behind the rocket
  m->SpawnPuff(actor->x, actor->y, actor->z);

  Actor *th = m->SpawnActor(actor->x-actor->px,
		     actor->y-actor->py,
		     actor->z, MT_SMOKE);

  th->pz = FRACUNIT;
  th->tics -= P_Random()&3;
  if (th->tics < 1)
    th->tics = 1;

  // adjust direction
  Actor *dest = actor->target;

  if (!dest || dest->health <= 0)
    return;

  // change angle
  exact = R_PointToAngle2 (actor->x,
			   actor->y,
			   dest->x,
			   dest->y);

  if (exact != actor->angle)
    {
      if (exact - actor->angle > 0x80000000)
        {
	  actor->angle -= TRACEANGLE;
	  if (exact - actor->angle < 0x80000000)
	    actor->angle = exact;
        }
      else
        {
	  actor->angle += TRACEANGLE;
	  if (exact - actor->angle > 0x80000000)
	    actor->angle = exact;
        }
    }

  exact = actor->angle>>ANGLETOFINESHIFT;
  actor->px = FixedMul (actor->info->speed, finecosine[exact]);
  actor->py = FixedMul (actor->info->speed, finesine[exact]);

  // change slope
  dist = P_AproxDistance (dest->x - actor->x,
			  dest->y - actor->y);

  dist = dist / actor->info->speed;

  if (dist < 1)
    dist = 1;
  slope = (dest->z+40*FRACUNIT - actor->z) / dist;

  if (slope < actor->pz)
    actor->pz -= FRACUNIT/8;
  else
    actor->pz += FRACUNIT/8;
}


void A_SkelWhoosh(Actor  *actor)
{
  if (!actor->target)
    return;
  A_FaceTarget (actor);
  // judgecutor:
  // CHECK ME!
  S_StartAttackSound(actor, sfx_skeswg);
}

void A_SkelFist(Actor  *actor)
{
  int         damage;

  if (!actor->target)
    return;

  A_FaceTarget (actor);

  if (actor->CheckMeleeRange())
    {
      damage = ((P_Random()%10)+1)*6;
      S_StartAttackSound(actor, sfx_skepch);
      actor->target->Damage(actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
static Actor *corpsehit, *vileobj;
static fixed_t  viletryx, viletryy;

bool PIT_VileCheck(Actor  *thing)
{
  int         maxdist;
  bool     check;

  if (!(thing->flags & MF_CORPSE) )
    return true;    // not a monster

  if (thing->tics != -1)
    return true;    // not lying still yet

  if (thing->info->raisestate == S_NULL)
    return true;    // monster doesn't have a raise state

  maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;

  if ( abs(thing->x - viletryx) > maxdist
       || abs(thing->y - viletryy) > maxdist )
    return true;            // not actually touching

  corpsehit = thing;
  corpsehit->px = corpsehit->py = 0;
  corpsehit->height <<= 2;
  check = corpsehit->CheckPosition(corpsehit->x, corpsehit->y);
  corpsehit->height >>= 2;

  if (!check)
    return true;            // doesn't fit here

  return false;               // got one, so stop checking
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase(Actor *actor)
{
  int xl, xh, yl, yh, bx, by;

  const mobjinfo_t *info;
  Actor *temp;
  Map *m = actor->mp;

  if (actor->movedir != DI_NODIR)
    {
      // check for corpses to raise
      viletryx = actor->x + actor->info->speed*xspeed[actor->movedir];
      viletryy = actor->y + actor->info->speed*yspeed[actor->movedir];

      xl = (viletryx - m->bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
      xh = (viletryx - m->bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
      yl = (viletryy - m->bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
      yh = (viletryy - m->bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

      vileobj = actor;
      for (bx=xl ; bx<=xh ; bx++)
        {
	  for (by=yl ; by<=yh ; by++)
            {
	      // Call PIT_VileCheck to check
	      // whether object is a corpse
	      // that canbe raised.
	      if (!m->BlockThingsIterator(bx,by,PIT_VileCheck))
                {
		  // got one!
		  temp = actor->target;
		  actor->target = corpsehit;
		  A_FaceTarget (actor);
		  actor->target = temp;

		  actor->SetState(S_VILE_HEAL1);
		  S_StartSound (corpsehit, sfx_slop);
		  info = corpsehit->info;

		  corpsehit->SetState(info->raisestate);
		  if( game.demoversion<129 )
		    corpsehit->height <<= 2;
		  else
                    {
		      corpsehit->height = info->height;
		      corpsehit->radius = info->radius;
                    }
		  corpsehit->flags = info->flags;
		  corpsehit->health = info->spawnhealth;
		  corpsehit->target = NULL;

		  return;
                }
            }
        }
    }

  // Return to normal attack.
  A_Chase(actor);
}


//
// A_VileStart
//
void A_VileStart(Actor *actor)
{
  S_StartAttackSound(actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//
void A_Fire(Actor *actor);

void A_StartFire(Actor *actor)
{
  S_StartSound(actor,sfx_flamst);
  A_Fire(actor);
}

void A_FireCrackle(Actor *actor)
{
  S_StartSound(actor,sfx_flame);
  A_Fire(actor);
}

void A_Fire(Actor *actor)
{
  unsigned  an;

  Actor *dest = actor->target;
  if (!dest)
    return;

  // don't move it if the vile lost sight
  if (!actor->mp->CheckSight(actor->owner, dest))
    return;

  an = dest->angle >> ANGLETOFINESHIFT;

  actor->UnsetPosition();
  actor->x = dest->x + FixedMul (24*FRACUNIT, finecosine[an]);
  actor->y = dest->y + FixedMul (24*FRACUNIT, finesine[an]);
  actor->z = dest->z;
  actor->SetPosition();
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget(Actor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget(actor);

  Actor *fire = actor->mp->SpawnActor(actor->target->x,
				      actor->target->x,
				      actor->target->z, MT_FIRE);
  actor->tracer = fire; // FIXME, tracer field should be removed. Owner? not good.
  fire->owner = actor;
  fire->target = actor->target;
  A_Fire(fire);
}




//
// A_VileAttack
//
void A_VileAttack(Actor *actor)
{  
  int an;

  if (!actor->target)
    return;

  A_FaceTarget(actor);

  if (!actor->mp->CheckSight(actor, actor->target) )
    return;

  S_StartSound (actor, sfx_barexp);
  actor->target->Damage(actor, actor, 20);
  actor->target->pz = 1000*FRACUNIT/actor->target->info->mass;

  an = actor->angle >> ANGLETOFINESHIFT;

  Actor *fire = actor->tracer;

  if (!fire)
    return;

  // move the fire between the vile and the player
  fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
  fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
  fire->RadiusAttack(actor, 70);
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.
//
#define FATSPREAD       (ANG90/8)

void A_FatRaise(Actor *actor)
{
  A_FaceTarget (actor);
  S_StartAttackSound(actor, sfx_manatk);
}


void A_FatAttack1(Actor *actor)
{
  Actor *mo;
  int    an;

  A_FaceTarget(actor);
  // Change direction  to ...
  actor->angle += FATSPREAD;
  actor->SpawnMissile(actor->target, MT_FATSHOT);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if (mo)
    {
      mo->angle += FATSPREAD;
      an = mo->angle >> ANGLETOFINESHIFT;
      mo->px = FixedMul (mo->info->speed, finecosine[an]);
      mo->py = FixedMul (mo->info->speed, finesine[an]);
    }
}

void A_FatAttack2(Actor *actor)
{
  Actor *mo;
  int         an;

  A_FaceTarget (actor);
  // Now here choose opposite deviation.
  actor->angle -= FATSPREAD;
  actor->SpawnMissile(actor->target, MT_FATSHOT);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->angle -= FATSPREAD*2;
      an = mo->angle >> ANGLETOFINESHIFT;
      mo->px = FixedMul (mo->info->speed, finecosine[an]);
      mo->py = FixedMul (mo->info->speed, finesine[an]);
    }
}

void A_FatAttack3(Actor *actor)
{
  Actor *mo;
  int         an;

  A_FaceTarget (actor);

  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->angle -= FATSPREAD/2;
      an = mo->angle >> ANGLETOFINESHIFT;
      mo->px = FixedMul (mo->info->speed, finecosine[an]);
      mo->py = FixedMul (mo->info->speed, finesine[an]);
    }
    
  mo = actor->SpawnMissile(actor->target, MT_FATSHOT);
  if(mo)
    {
      mo->angle += FATSPREAD/2;
      an = mo->angle >> ANGLETOFINESHIFT;
      mo->px = FixedMul (mo->info->speed, finecosine[an]);
      mo->py = FixedMul (mo->info->speed, finesine[an]);
    }
}


//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED              (20*FRACUNIT)

void A_SkullAttack(Actor *actor)
{
  Actor *dest;
  angle_t             an;
  int                 dist;

  if (!actor->target)
    return;

  dest = actor->target;
  actor->flags |= MF_SKULLFLY;
  S_StartScreamSound(actor, actor->info->attacksound);
  A_FaceTarget (actor);
  an = actor->angle >> ANGLETOFINESHIFT;
  actor->px = FixedMul (SKULLSPEED, finecosine[an]);
  actor->py = FixedMul (SKULLSPEED, finesine[an]);
  dist = P_AproxDistance (dest->x - actor->x, dest->y - actor->y);
  dist = dist / SKULLSPEED;

  if (dist < 1)
    dist = 1;
  actor->pz = (dest->z+(dest->height>>1) - actor->z) / dist;
}


//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void A_PainShootSkull(Actor *actor, angle_t angle)
{
  fixed_t     x;
  fixed_t     y;
  fixed_t     z;

  Actor *newmobj;
  angle_t     an;
  int         prestep;


  /*  --------------- SKULL LIMITE CODE -----------------
      int         count;
      thinker_t*  currentthinker;

      // count total number of skull currently on the level
      count = 0;

      currentthinker = thinkercap.next;
      while (currentthinker != &thinkercap)
      {
      if (   (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
      && ((Actor *)currentthinker)->type == MT_SKULL)
      count++;
      currentthinker = currentthinker->next;
      }

      // if there are allready 20 skulls on the level,
      // don't spit another one
      if (count > 20)
      return;
      ---------------------------------------------------
  */

  // okay, there's place for another one
  an = angle >> ANGLETOFINESHIFT;

  prestep =
    4*FRACUNIT
    + 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;

  x = actor->x + FixedMul (prestep, finecosine[an]);
  y = actor->y + FixedMul (prestep, finesine[an]);
  z = actor->z + 8*FRACUNIT;

  newmobj = actor->mp->SpawnActor(x, y, z, MT_SKULL);

  // Check for movements.
  if (!newmobj->TryMove(newmobj->x, newmobj->y, false))
    {
      // kill it immediately
      newmobj->Damage(actor,actor,10000);
      return;
    }

  newmobj->target = actor->target;
  A_SkullAttack (newmobj);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//
void A_PainAttack(Actor *actor)
{
  if (!actor->target)
    return;

  A_FaceTarget (actor);
  A_PainShootSkull (actor, actor->angle);
}


void A_PainDie(Actor *actor)
{
  A_Fall (actor);
  A_PainShootSkull (actor, actor->angle+ANG90);
  A_PainShootSkull (actor, actor->angle+ANG180);
  A_PainShootSkull (actor, actor->angle+ANG270);
}


void A_Scream(Actor *actor)
{
  int         sound;

  switch (actor->info->deathsound)
    {
    case 0:
      return;

    case sfx_podth1:
    case sfx_podth2:
    case sfx_podth3:
      sound = sfx_podth1 + P_Random ()%3;
      break;

    case sfx_bgdth1:
    case sfx_bgdth2:
      sound = sfx_bgdth1 + P_Random ()%2;
      break;

    default:
      sound = actor->info->deathsound;
      break;
    }

  // Check for bosses.
  if (actor->type==MT_SPIDER
      || actor->type == MT_CYBORG)
    {
      // full volume
      S_StartAmbSound(sound);
    }
  else
    S_StartScreamSound(actor, sound);
}


void A_XScream(Actor *actor)
{
  S_StartScreamSound(actor, sfx_slop);
}

void A_Pain(Actor *actor)
{
  if (actor->info->painsound)
    S_StartScreamSound(actor, actor->info->painsound);
}


//
//  A dying thing falls to the ground (monster deaths)
//
void A_Fall(Actor *actor)
{
  // actor is on ground, it can be walked over
  if (!cv_solidcorpse.value)
    actor->flags &= ~MF_SOLID;
  if( game.demoversion >= 131 )
    {
      actor->flags   |= MF_CORPSE|MF_DROPOFF;
      actor->height >>= 2;
      actor->radius -= (actor->radius>>4);      //for solid corpses
      actor->health = actor->info->spawnhealth>>1;
    }

  // So change this if corpse objects
  // are meant to be obstacles.
}


//
// A_Explode
//
void A_Explode(Actor *actor)
{
  int damage = 128;

  switch(actor->type)
    {
    case MT_FIREBOMB: // Time Bombs
      actor->z += 32*FRACUNIT;
      actor->flags &= ~MF_SHADOW;
      break;
    case MT_MNTRFX2: // Minotaur floor fire
      damage = 24;
      break;
    case MT_SOR2FX1: // D'Sparil missile
      damage = 80+(P_Random()&31);
      break;
    default:
      break;
    }

  actor->RadiusAttack(actor->owner, damage);
  actor->HitFloor();
}

//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
void A_BossDeath(Actor *mo)
{
  mo->mp->BossDeath(mo);
}

//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie(Actor *mo)
{
  A_Fall(mo);

  mo->mp->BossDeath(mo);
}

void A_Hoof(Actor *mo)
{
  S_StartSound (mo, sfx_hoof);
  A_Chase (mo);
}

void A_Metal(Actor *mo)
{
  S_StartSound (mo, sfx_metal);
  A_Chase (mo);
}

void A_BabyMetal(Actor *mo)
{
  S_StartSound (mo, sfx_bspwlk);
  A_Chase (mo);
}

void A_OpenShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbopn);
}

void A_LoadShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbload);
}

void A_ReFire(PlayerPawn *p, pspdef_t *psp);

void A_CloseShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbcls);
  A_ReFire(p, psp);
}

void A_BrainAwake(Actor *mo)
{
  S_StartAmbSound(sfx_bossit);
}


void A_BrainPain(Actor *mo)
{
  S_StartAmbSound(sfx_bospn);
}


void A_BrainScream(Actor *mo)
{
  int         x;
  int         y;
  int         z;
  Actor *th;

  for (x=mo->x - 196*FRACUNIT ; x< mo->x + 320*FRACUNIT ; x+= FRACUNIT*8)
    {
      y = mo->y - 320*FRACUNIT;
      z = 128 + P_Random()*2*FRACUNIT;
      th = mo->mp->SpawnActor(x,y,z, MT_ROCKET);
      th->pz = P_Random()*512;

      th->SetState(S_BRAINEXPLODE1);

      th->tics -= P_Random()&7;
      if (th->tics < 1)
	th->tics = 1;
    }

  S_StartAmbSound(sfx_bosdth);
}



void A_BrainExplode(Actor *mo)
{
  int         x;
  int         y;
  int         z;
  Actor *th;

  x = (P_SignedRandom()<<11)+mo->x;
  y = mo->y;
  z = 128 + P_Random()*2*FRACUNIT;
  th = mo->mp->SpawnActor(x,y,z, MT_ROCKET);
  th->pz = P_Random()*512;

  th->SetState(S_BRAINEXPLODE1);

  th->tics -= P_Random()&7;
  if (th->tics < 1)
    th->tics = 1;
}


void A_BrainDie(Actor *mo)
{
  mo->mp->BossDeath(mo);
}

void A_BrainSpit(Actor *mo)
{
  Actor *targ;
  Actor *newmobj;
  Map *m = mo->mp;

  static int  easy = 0;

  easy ^= 1;
  if (game.skill <= sk_easy && (!easy))
    return;

  int n = m->braintargets.size();
  if (n > 0) 
    {
      // shoot a cube at current target
      targ = m->braintargets[m->braintargeton];
      m->braintargeton = (m->braintargeton+1) % n;
        
      // spawn brain missile
      newmobj = mo->SpawnMissile(targ, MT_SPAWNSHOT);
      if (newmobj)
        {
	  newmobj->target = targ;
	  newmobj->reactiontime =
	    ((targ->y - mo->y)/newmobj->py) / newmobj->state->tics;
        }

      S_StartAmbSound(sfx_bospit);
    }
}



void A_SpawnFly(Actor *mo);

// travelling cube sound
void A_SpawnSound(Actor *mo)
{
  S_StartSound (mo,sfx_boscub);
  A_SpawnFly(mo);
}

void A_SpawnFly(Actor *mo)
{
  Actor *newmobj;
  Actor *fog;
  Actor *targ;
  int         r;
  mobjtype_t  type;

  if (--mo->reactiontime)
    return; // still flying

  targ = mo->target;

  // First spawn teleport fog.
  fog = mo->mp->SpawnActor(targ->x, targ->y, targ->z, MT_SPAWNFIRE);
  S_StartSound (fog, sfx_telept);

  // Randomly select monster to spawn.
  r = P_Random ();

  // Probability distribution (kind of :),
  // decreasing likelihood.
  if ( r<50 )
    type = MT_TROOP;
  else if (r<90)
    type = MT_SERGEANT;
  else if (r<120)
    type = MT_SHADOWS;
  else if (r<130)
    type = MT_PAIN;
  else if (r<160)
    type = MT_HEAD;
  else if (r<162)
    type = MT_VILE;
  else if (r<172)
    type = MT_UNDEAD;
  else if (r<192)
    type = MT_BABY;
  else if (r<222)
    type = MT_FATSO;
  else if (r<246)
    type = MT_KNIGHT;
  else
    type = MT_BRUISER;

  newmobj = mo->mp->SpawnActor(targ->x, targ->y, targ->z, type);

  if (newmobj->LookForPlayers(true))
    newmobj->SetState(newmobj->info->seestate);

  // telefrag anything in this spot
  newmobj->TeleportMove(newmobj->x, newmobj->y);

  // remove self (i.e., cube).
  mo->Remove();
}


void A_PlayerScream(Actor *mo)
{
  // Default death sound.
  int         sound = sfx_pldeth;

  if ((game.mode == commercial) && (mo->health < -50))
    {
      // IF THE PLAYER DIES
      // LESS THAN -50% WITHOUT GIBBING
      sound = sfx_pdiehi;
    }
  S_StartScreamSound(mo, sound);
}

