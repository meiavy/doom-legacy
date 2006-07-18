// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Archiving: SaveGame I/O.

#include "doomdef.h"
#include "doomdata.h"

#include "command.h"
#include "console.h"

#include "m_archive.h"

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_team.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "n_interface.h"

#include "p_spec.h"
#include "r_poly.h"

#include "p_acs.h"
#include "r_data.h"
#include "r_sprite.h"
#include "t_vari.h"
#include "t_script.h"
#include "t_parse.h"
#include "m_random.h"
#include "m_misc.h"
#include "m_swap.h"
#include "m_menu.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "tables.h"

#include "hud.h"

// NOTE! The Map *mp is not saved for Thinkers in general, because it can be deduced
// from the context. However, some thinkers are not always associated with a Map.
// Hence they must save the Map reference as well.


enum consistency_marker_t
{
  // consistency markers in the savegame
  MARK_GROUP  = 0x71717171,
  MARK_MAP    = 0x8ae51d73,
  MARK_POLYOBJ = 0x3c3c3c3c,
  MARK_SCRIPT = 0x37c4fe01,
  MARK_THINK  = 0x1c02fa39,
  MARK_MISC   = 0xabababab,

  // this special marker is used to denote the end of a collection of objects
  MARK_END = 0xffffffff,
};


//==============================================
// Marshalling functions for various Thinkers
//==============================================

int acs_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = ((byte *)ip - mp->ActionCodeBase)); // byte offset
      a << (temp = line ? (line - mp->lines) : -1);
      Thinker::Serialize(activator, a);
      a.Write((byte *)stak, sizeof(stak));
      a.Write((byte *)vars, sizeof(vars));
    }
  else
    {
      a << temp;
      ip = (int *)(&mp->ActionCodeBase[temp]);
      a << temp;
      line = (temp == -1) ? NULL : &mp->lines[temp];
      activator = (Actor *)Thinker::Unserialize(a);
      a.Read((byte *)stak, sizeof(stak));
      a.Read((byte *)vars, sizeof(vars));
    }

  a << side << number << infoIndex << delayCount << stackPtr;
  return 0;
}

int sectoreffect_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      if (!sector)
	I_Error("Sectoreffect with no sector!\n");
      a << (temp = sector - mp->sectors);
    }
  else
    {
      a << temp;
      if (temp >= mp->numsectors)
	I_Error("Invalid sector for a Sectoreffect!\n");
      sector = &mp->sectors[temp];
    }

  return 0;
}

int lightfx_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << type << count << maxlight << minlight << maxtime << mintime;
  return 0;
}

int phasedlight_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << base << index;
  return 0;
}

int floor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << type << crush << speed << modelsec << texture << destheight;
  return 0;
}

int stair_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << state << resetcount << wait << stepdelay << speed << destheight << originalheight << delayheight << stepdelta;
  return 0;
}

int elevator_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = sector->ceilingdata = this;

  a << type << crush << floordest << ceilingdest << floorspeed << ceilingspeed;
  return 0;
}

int floorwaggle_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << state << phase << freq << baseheight << amp << maxamp << ampdelta << wait;
  return 0;
}

int plat_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->floordata = this;
      mp->AddActivePlat(this);
    }

  a << type << status;
  a << speed << low << high << wait << count;
  
  return 0;
}

int ceiling_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->ceilingdata = this;
      mp->AddActiveCeiling(this);
    }

  a << type << crush << speed << destheight << modelsec << texture;

  return 0;
}

int crusher_t::Marshal(LArchive &a)
{
  ceiling_t::Marshal(a);
  a << downspeed << upspeed << bottomheight << topheight;
  return 0;
}

int vdoor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->ceilingdata = this;

  a << type << direction << topheight << speed << topwait << topcount << boomlighttag;
  return 0;
}

int button_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = line ? (line - mp->lines) : -1);
    }
  else
    {
      a << temp;
      line = (temp == -1) ? NULL : &mp->lines[temp];
      soundorg = &line->frontsector->soundorg;
    }

  a << texture << timer << where;
  return 0;
}

int scroll_t::Marshal(LArchive &a)
{
  a << type << affectee << vx << vy;
  a << last_height << accel << vdx << vdy;

  int temp;
  if (a.IsStoring())
    {
      a << (temp = control ? (control - mp->sectors) : -1);
    }
  else
    {
      a << temp;
      control = (temp == -1) ? NULL : &mp->sectors[temp];
    }
  return 0;
}

int pusher_t::Marshal(LArchive &a)
{
  a << type << x_mag << y_mag << magnitude << radius;
  a << x << y << affectee;
  if (a.IsStoring())
    Thinker::Serialize(source, a);
  else
    source = (DActor *)Thinker::Unserialize(a);
  return 0;
}


int polyobject_t::Marshal(LArchive &a)
{
  a << polyobj;
  return 0;
}

int polyrotator_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << speed << dist;
  return 0;
}

int polymover_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << speed << dist << ang << xs << ys;
  return 0;
}

int polydoor_rot_t::Marshal(LArchive &a)
{
  polyrotator_t::Marshal(a);
  a << closing << tics << waitTics << totalDist;
  return 0;
}

int polydoor_slide_t::Marshal(LArchive &a)
{
  polymover_t::Marshal(a);
  a << closing << tics << waitTics << totalDist;
  return 0;
}


int presentation_t::Serialize(presentation_t *p, LArchive &a)
{
  // much simpler than the Thinker serialization.
  int temp;
  //CONS_Printf("serializing a presentation\n");
  if (p)
    p->Marshal(a); // handles the type id as well.
  else
    a << (temp = 0);
  return 0;
}

presentation_t *presentation_t::Unserialize(LArchive &a)
{
  presentation_t *p;
  int temp;
  a << temp; // read the type id
  //CONS_Printf("unserializing a presentation, %d\n", temp);
  if (temp == 0)
    return NULL;
  else
    p = new spritepres_t(NULL, 0);
  /*
  else
    p = new modelpres_t(NULL);
  */

  p->Marshal(a);

  return p;
}

int spritepres_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = 1); // type id
      a << (temp = (info - mobjinfo));
      a << (temp = (state - states));
    }
  else
    {
      a << temp;
      info = mobjinfo + temp;
      a << temp;
      SetFrame(&states[temp]);
    }

  a << color << animseq;
  return 0;
}

int modelpres_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = 2); // type id
      // TODO store the model name
    }
  else
    {
      // and restore it: mdl = models.Get("xxx");
    }

  a << color << animseq;
  return 0;
}

int Actor::Marshal(LArchive &a)
{
  int temp;
  a << pos << vel;
  a << yaw << pitch;
  a << mass << radius << height;
  a << health;

  a << flags << flags2 << eflags;

  a << tid << special;
  for (int i=0; i<5; i++)
    a << args[i];

  a << reactiontime << floorclip;
  a << team;

  if (a.IsStoring())
    {
      a << (temp = spawnpoint ? (spawnpoint - mp->mapthings) : -1);
      Thinker::Serialize(owner, a);
      Thinker::Serialize(target, a);
      presentation_t::Serialize(pres, a);
    }
  else
    {
      a << temp;
      spawnpoint = (temp == -1) ? NULL : &mp->mapthings[temp];

      owner  = (Actor *)Thinker::Unserialize(a);
      target = (Actor *)Thinker::Unserialize(a);
      pres = presentation_t::Unserialize(a);

      if (mp)
	{
	  if (spawnpoint)
	    spawnpoint->mobj = this;
	  CheckPosition(pos.x, pos.y);
	  SetPosition();
	}
    }

  return 0;
}


int DActor::Marshal(LArchive &a)
{ 
  // NOTE is it really worth the effort to do this delta-coding?

  enum dactor_diff_e
  {
    MD_SPAWNPOINT = 0x000001,
    MD_TYPE       = 0x000002,
    MD_XY         = 0x000004,
    MD_Z          = 0x000008,
    MD_MOM        = 0x000010,
    MD_MASS       = 0x000020,
    MD_RADIUS     = 0x000040,
    MD_HEIGHT     = 0x000080,
    MD_HEALTH     = 0x000100,

    MD_FLAGS      = 0x000200,
    MD_FLAGS2     = 0x000400,
    MD_EFLAGS     = 0x000800,

    MD_TID        = 0x001000,
    MD_SPECIAL    = 0x002000,
    MD_RTIME      = 0x004000,
    MD_FLOORCLIP  = 0x008000,
    MD_TEAM       = 0x010000,

    MD_STATE      = 0x020000,
    MD_TICS       = 0x040000,
    MD_MOVEDIR    = 0x080000,
    MD_MOVECOUNT  = 0x100000,
    MD_THRESHOLD  = 0x200000,
    MD_LASTLOOK   = 0x400000,
    MD_SPECIAL1   = 0x800000,
    MD_SPECIAL2  = 0x1000000,

    MD_TARGET    = 0x2000000,
    MD_OWNER     = 0x4000000,
  };

  short stemp;
  int i;

  unsigned diff;

  if (a.IsStoring())
    {
      // find the differences
      if (spawnpoint)
	{
	  diff = MD_SPAWNPOINT;
    
	  if ((pos.x != spawnpoint->x) || (pos.y != spawnpoint->y) ||
	      (yaw != unsigned(ANG45 * (spawnpoint->angle/45))))
	    diff |= MD_XY;

	  if (type != spawnpoint->type)
	    diff |= MD_TYPE;
	}
      else
	{
	  // not a map spawned thing so make it from scratch
	  diff = MD_XY | MD_TYPE;
	}

      // not the default but the most probable
      if (pos.z != floorz)             diff |= MD_Z;
      if (vel != vec_t<fixed_t>(0,0,0)) diff |= MD_MOM;
      if (mass   != info->mass)        diff |= MD_MASS;
      if (radius != info->radius)      diff |= MD_RADIUS;
      if (height != info->height)      diff |= MD_HEIGHT;
      if (health != info->spawnhealth) diff |= MD_HEALTH;
      if (flags  != info->flags)       diff |= MD_FLAGS;
      if (flags2 != info->flags2)      diff |= MD_FLAGS2;
      if (eflags)                      diff |= MD_EFLAGS;
 
      if (tid)       diff |= MD_TID;
      if (special)   diff |= MD_SPECIAL;
      if (reactiontime != info->reactiontime) diff |= MD_RTIME;
      if (floorclip != 0) diff |= MD_FLOORCLIP;
      if (team) diff |= MD_TEAM;

      if (state-states != info->spawnstate)   diff |= MD_STATE;
      if (tics         != state->tics)        diff |= MD_TICS;

      if (movedir)        diff |= MD_MOVEDIR;
      if (movecount)      diff |= MD_MOVECOUNT;
      if (threshold)      diff |= MD_THRESHOLD;
      if (lastlook != -1) diff |= MD_LASTLOOK;
      if (special1)  diff |= MD_SPECIAL1;
      if (special2)  diff |= MD_SPECIAL2;

      if (owner)   diff |= MD_OWNER;
      if (target)  diff |= MD_TARGET;

      a << diff;

      if (diff & MD_SPAWNPOINT)
	{
	  stemp = short(spawnpoint - mp->mapthings);
	  a << stemp;
	}

#if __GNUC__ >= 4
      if (diff & MD_TYPE) a << (short &)type;
#else
      if (diff & MD_TYPE) a << short(type);
#endif
      if (diff & MD_XY)   a << pos.x << pos.y << yaw;
      if (diff & MD_Z)    a << pos.z;
      if (diff & MD_MOM)  a << vel;

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_TID)    a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}
      if (diff & MD_RTIME)     a << reactiontime;
      if (diff & MD_FLOORCLIP) a << floorclip;
      if (diff & MD_TEAM)      a << team;

      if (diff & MD_STATE)
	{
	  stemp = short(state - states);
	  a << stemp;
	}
      if (diff & MD_TICS)      a << tics;

      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;
      if (diff & MD_SPECIAL1)  a << special1;
      if (diff & MD_SPECIAL2)  a << special2;

      if (diff & MD_OWNER)  Thinker::Serialize(owner, a);
      if (diff & MD_TARGET) Thinker::Serialize(target, a);
    }
  else
    {
      // retrieving
      a << diff;

      if (diff & MD_SPAWNPOINT)
	{
	  a << stemp;
	  spawnpoint = mp->mapthings + stemp;
	  mp->mapthings[stemp].mobj = this;
	}

      if (diff & MD_TYPE)
	{
	  a << stemp;
	  type = mobjtype_t(stemp);
	}
      else
	type = mobjtype_t(spawnpoint->type); // Map::LoadThings() should set it to the correct value
      info = &mobjinfo[type];

      {
	// use info for init, correct later
	mass         = info->mass;
	radius       = info->radius;
	height       = info->height;
	health       = info->spawnhealth;
	flags        = info->flags;
	flags2       = info->flags2;
	reactiontime = info->reactiontime;
	state        = &states[info->spawnstate];
      }

      if (diff & MD_XY)
	a << pos.x << pos.y << yaw;
      else
	{
	  pos.x = spawnpoint->x;
	  pos.y = spawnpoint->y;
	  yaw = ANG45 * (spawnpoint->angle/45);
	}

      if (diff & MD_Z)
	a << pos.z;

      if (diff & MD_MOM)
	a << vel; // else zero (by constructor)

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_TID)      a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}
      if (diff & MD_RTIME)     a << reactiontime;
      if (diff & MD_FLOORCLIP) a << floorclip;
      if (diff & MD_TEAM)      a << team;

      if (diff & MD_STATE)
	{
	  a << stemp;
	  state = &states[stemp];
	}
      if (diff & MD_TICS)
	a << tics;
      else
	tics = state->tics;

      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;
      if (diff & MD_SPECIAL1)  a << special1;
      if (diff & MD_SPECIAL2)  a << special2;

      if (diff & MD_OWNER)  owner  = (Actor *)Thinker::Unserialize(a);
      if (diff & MD_TARGET) target = (Actor *)Thinker::Unserialize(a);

      // set sprev, snext, bprev, bnext, subsector
      if (mp)
	{
	  CheckPosition(pos.x, pos.y); // TEST, sets tmfloorz, tmceilingz
	  SetPosition();
	}

      if (!(diff & MD_Z))
	pos.z = floorz;

      // TODO simplified presentation loading for DActors for now (only sprites)
      pres = new spritepres_t(info, 0);
      pres->SetFrame(state);
    }

  return 0;
}


int Pawn::Marshal(LArchive & a)
{
  Actor::Marshal(a);

  a << color;
  a << maxhealth << speed;
  a << attackphase;

  int temp;

  if (a.IsStoring())
    {
      a << (temp = pinfo ? (pinfo - pawndata) : -1);
      Thinker::Serialize(attacker, a);
    }
  else
    {
      a << temp;
      pinfo = (temp == -1) ? NULL : &pawndata[temp];
      attacker = (Actor *)Thinker::Unserialize(a);
    }

  return 0;
}


int PlayerPawn::Marshal(LArchive &a)
{
  Pawn::Marshal(a);

  int i, n, diff;

  enum player_diff
  {
    PD_POWERS       = 0x0001,
    PD_PMASK        = 0xFFFF, // 12 powers now in total (room for 16)

    PD_REFIRE      = 0x010000,
    PD_MORPHTICS   = 0x020000,

    PD_ATTACKDWN   = 0x100000,
    PD_USEDWN      = 0x200000,
    PD_JMPDWN      = 0x400000
  };

  if (a.IsStoring())
    {
      for (i=0; i<NUMPSPRITES; i++)
	{
	  a << (n = psprites[i].state ? (psprites[i].state - weaponstates) : -1);
	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      // inventory is closed.
      a << (n = inventory.size());
      for (i=0; i<n; i++)
	a << inventory[i].type << inventory[i].count;

      diff = 0;
      for (i=0; i<NUMWEAPONS; i++)
	if (weaponowned[i])
	  diff |= (1 << i);
      a << diff; // we have already 32 weapon types. whew!

      diff = 0;
      for (i=0; i<NUMPOWERS; i++)
	if (powers[i])
	  diff |= PD_POWERS << i;

      if (refire)      diff |= PD_REFIRE;
      if (morphTics)   diff |= PD_MORPHTICS;

      // booleans
      if (attackdown) diff |= PD_ATTACKDWN;
      if (usedown)    diff |= PD_USEDWN;
      if (jumpdown)   diff |= PD_JMPDWN;

      a << diff;

      for (i=0; i<NUMPOWERS; i++)
	if (diff & (PD_POWERS << i))
	  a << powers[i];

      if (diff & PD_REFIRE) a << refire;
      if (diff & PD_MORPHTICS) a << morphTics;

      a << (int &)pendingweapon << (int &)readyweapon;
    }
  else
    {
      // loading
      for (i=0; i<NUMPSPRITES; i++)
	{
	  a << n;	  
	  psprites[i].state = (n == -1) ? NULL : &weaponstates[n];
	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      a << n;
      inventory.resize(n);
      for (i=0; i<n; i++)
	a << inventory[i].type << inventory[i].count;

      a << diff;
      for (i=0; i<NUMWEAPONS; i++)
	weaponowned[i] = (diff & (1 << i));

      a << diff;
      for (i=0; i<NUMPOWERS; i++)
	if (diff & (PD_POWERS << i))
	  a << powers[i];

      if (diff & PD_REFIRE) a << refire;
      if (diff & PD_MORPHTICS) a << morphTics;

      attackdown   = diff & PD_ATTACKDWN;
      usedown      = diff & PD_USEDWN;
      jumpdown     = diff & PD_JMPDWN;

      weaponinfo = (powers[pw_weaponlevel2] ? wpnlev2info : wpnlev1info);

      a << n; pendingweapon = weapontype_t(n);
      a << n; readyweapon = weapontype_t(n);
    }

  // non-coded stuff (just read/write the numbers)
  a << pclass;
  a << fly_zspeed;
  a << keycards;
  a << cheats;

  for (i=0; i<NUMAMMO; i++)
    {
      a << ammo[i];
      a << maxammo[i];
    }

  a << toughness;
  for (i=0; i<NUMARMOR; i++)
    a << armorfactor[i] << armorpoints[i];

  a << specialsector << extralight << fixedcolormap;

  return 0;
}




//==============================================
//   Script serialization stuff
//==============================================

int svariable_t::Serialize(LArchive &a)
{
  a << name;
  a << type;
  switch (type)
    {
    case svt_string:
      a << value.s;
      break;
    case svt_int:
    case svt_fixed:
      a << value.i;
      break;
    case svt_actor:
      Thinker::Serialize(value.mobj, a);
      break;
    }

  return 0;
}


int svariable_t::Unserialize(LArchive &a)
{
  a << name;
  Z_ChangeTag(name, PU_LEVEL);
  a << type;
  switch (type)
    {
    case svt_string:
      a << value.s;
      Z_ChangeTag(value.s, PU_LEVEL);
      break;
    case svt_int:
    case svt_fixed:
      a << value.i;
      break;
    case svt_actor:
      value.mobj = (Actor *)Thinker::Unserialize(a);
      break;
    }

  return 0;
}      




//==============================================
//  Map serialization
//==============================================

enum mapdiff_e
{
    // sectors
    // diff
    SD_FLOORHT  = 0x01,
    SD_CEILHT   = 0x02,
    SD_FLOORPIC = 0x04,
    SD_CEILPIC  = 0x08,
    SD_LIGHT    = 0x10,
    SD_SPECIAL  = 0x20,
    SD_TAG      = 0x40,
    SD_DIFF2    = 0x80,

    // diff2
    SD_FXOFFS    = 0x01,
    SD_FYOFFS    = 0x02,
    SD_CXOFFS    = 0x04,
    SD_CYOFFS    = 0x08,
    SD_STAIRLOCK = 0x10,
    SD_PREVSEC   = 0x20,
    SD_NEXTSEC   = 0x40,
    SD_SEQTYPE   = 0x80,

    // line- and sidedefs
    // diff
    LD_FLAG     = 0x01,
    LD_SPECIAL  = 0x02,
    LD_ARGS     = 0x04,
    LD_S1TEXOFF = 0x08,
    LD_S1TOPTEX = 0x10,
    LD_S1BOTTEX = 0x20,
    LD_S1MIDTEX = 0x40,
    LD_DIFF2    = 0x80,

    // diff2
    LD_S2TEXOFF = 0x01,
    LD_S2TOPTEX = 0x02,
    LD_S2BOTTEX = 0x04,
    LD_S2MIDTEX = 0x08,
};




int Map::Serialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, j, n;

  a.Marker(MARK_MAP);

  a << lumpname;
  // TODO save map md5 checksum, to make sure the correct map is loaded
  a << starttic << maptic;
  a << kills << items << secrets; // conceivably scripts could change these

  //----------------------------------------------
  // record the changes in static geometry (as compared to wad)
  // "reload" the map to check for differences
  // TODO helluvalot of unneeded stuff is saved because of linedef/sector special mappings...
  // Perhaps we should really fake-setup the comparison map... or perhaps we'll just live with it.
  int statsec = 0, statline = 0;

  byte diff, diff2;

  mapsector_t *ms = (mapsector_t *)fc.CacheLumpNum(lumpnum + LUMP_SECTORS, PU_CACHE);
  sector_t    *ss = sectors;

  for (i = 0; i<numsectors ; i++, ss++, ms++)
    {
      diff = diff2 = 0;
      if (ss->floorheight != SHORT(ms->floorheight))
	diff |= SD_FLOORHT;
      if (ss->ceilingheight != SHORT(ms->ceilingheight))
	diff |= SD_CEILHT;

      if (ss->floorpic != tc.GetID(ms->floorpic, TEX_floor))
	diff |= SD_FLOORPIC;
      if (ss->ceilingpic != tc.GetID(ms->ceilingpic, TEX_floor))
	diff |= SD_CEILPIC;

      if (ss->lightlevel != SHORT(ms->lightlevel)) diff |= SD_LIGHT;
      if (ss->special != SHORT(ms->special))       diff |= SD_SPECIAL;
      if (ss->tag != SHORT(ms->tag))               diff |= SD_TAG;
      // floortype?
      if (ss->floor_xoffs != 0)   diff2 |= SD_FXOFFS;
      if (ss->floor_yoffs != 0)   diff2 |= SD_FYOFFS;
      if (ss->ceiling_xoffs != 0) diff2 |= SD_CXOFFS;
      if (ss->ceiling_yoffs != 0) diff2 |= SD_CYOFFS;
      if (ss->stairlock < 0)      diff2 |= SD_STAIRLOCK;
      if (ss->nextsec != -1)      diff2 |= SD_NEXTSEC;
      if (ss->prevsec != -1)      diff2 |= SD_PREVSEC;
      if (ss->seqType)          diff2 |= SD_SEQTYPE;

      if (diff2)
	diff |= SD_DIFF2;

      if (diff)
        {
	  statsec++;

	  a << i;
	  a << diff;

	  if (diff & SD_DIFF2)
	    a << diff2;
	  if (diff & SD_FLOORHT) a << ss->floorheight;
	  if (diff & SD_CEILHT)  a << ss->ceilingheight;

	  if (diff & SD_FLOORPIC)
	    a.Write((byte *)tc[ss->floorpic]->GetName(), 8);
	  if (diff & SD_CEILPIC)
	    a.Write((byte *)tc[ss->ceilingpic]->GetName(), 8);

	  if (diff & SD_LIGHT)    a << ss->lightlevel;
	  if (diff & SD_SPECIAL)  a << ss->special;
	  if (diff & SD_TAG)      a << ss->tag;

	  if (diff2 & SD_FXOFFS)  a << ss->floor_xoffs;
	  if (diff2 & SD_FYOFFS)  a << ss->floor_yoffs;
	  if (diff2 & SD_CXOFFS)  a << ss->ceiling_xoffs;
	  if (diff2 & SD_CYOFFS)  a << ss->ceiling_yoffs;
	  if (diff2 & SD_STAIRLOCK) a << ss->stairlock;
	  if (diff2 & SD_NEXTSEC)   a << ss->nextsec;
	  if (diff2 & SD_PREVSEC)   a << ss->prevsec;
	  if (diff2 & SD_SEQTYPE)   a << ss->seqType;
        }
    }
  a << (temp = MARK_END);

  doom_maplinedef_t *mld = (doom_maplinedef_t *)fc.CacheLumpNum(lumpnum + LUMP_LINEDEFS, PU_CACHE);
  hex_maplinedef_t *hld = (hex_maplinedef_t *)mld;
  mapsidedef_t *msd = (mapsidedef_t *)fc.CacheLumpNum(lumpnum + LUMP_SIDEDEFS, PU_CACHE);
  line_t *li = lines;
  side_t *si;

  // do lines
  for (i=0 ; i<numlines ; i++, li++)
    {
      diff = diff2 = 0;

      if (hexen_format)
	{
	  if (li->flags != SHORT(hld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(hld->special))
	    diff |= LD_SPECIAL;
	  for (j=0; j<5; j++)
	    if (li->args[j] != hld->args[j])
	      diff |= LD_ARGS;
	  hld++;
	}
      else
	{
	  if (li->flags != SHORT(mld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(mld->special))
	    diff |= LD_SPECIAL;
	  // TODO save tag?
	  mld++;
	}

      if (li->sideptr[0])
        {
	  si = li->sideptr[0];
	  temp = si - sides; // sidedef number

	  if (si->textureoffset != SHORT(msd[temp].textureoffset))
	    diff |= LD_S1TEXOFF;

	  // do texture diffing by comparing names to be sure
	  if (si->toptexture && strncmp(tc[si->toptexture]->GetName(), msd[temp].toptexture, 8))
	    diff |= LD_S1TOPTEX;
	  if (si->bottomtexture && strncmp(tc[si->bottomtexture]->GetName(), msd[temp].bottomtexture, 8))
	    diff |= LD_S1BOTTEX;
	  if (si->midtexture && strncmp(tc[si->midtexture]->GetName(), msd[temp].midtexture, 8))
	    diff |= LD_S1MIDTEX;
        }
      if (li->sideptr[1])
        {
	  si = li->sideptr[1];
	  temp = si - sides; // sidedef number

	  if (si->textureoffset != SHORT(msd[temp].textureoffset))
	    diff2 |= LD_S2TEXOFF;

	  if (si->toptexture && strncmp(tc[si->toptexture]->GetName(), msd[temp].toptexture, 8))
	    diff2 |= LD_S2TOPTEX;
	  if (si->bottomtexture && strncmp(tc[si->bottomtexture]->GetName(), msd[temp].bottomtexture, 8))
	    diff2 |= LD_S2BOTTEX;
	  if (si->midtexture && strncmp(tc[si->midtexture]->GetName(), msd[temp].midtexture, 8))
	    diff2 |= LD_S2MIDTEX;

	  if (diff2)
	    diff |= LD_DIFF2;
        }

      if (diff)
        {
	  statline++;

	  a << i;
	  a << diff;
	  if (diff & LD_DIFF2  )  a << diff2;
	  if (diff & LD_FLAG   )  a << li->flags;
	  if (diff & LD_SPECIAL)  a << li->special;
	  if (diff & LD_ARGS)
	    for (j=0; j<5; j++)
	      a << li->args[j];

	  si = li->sideptr[0];
	  if (diff & LD_S1TEXOFF) a << si->textureoffset;
	  if (diff & LD_S1TOPTEX) a << si->toptexture;
	  if (diff & LD_S1BOTTEX) a << si->bottomtexture;
	  if (diff & LD_S1MIDTEX) a << si->midtexture;

	  si = li->sideptr[1];
	  if (diff2 & LD_S2TEXOFF) a << si->textureoffset;
	  if (diff2 & LD_S2TOPTEX) a << si->toptexture;
	  if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
	  if (diff2 & LD_S2MIDTEX) a << si->midtexture;
        }
    }
  a << (temp = MARK_END);
  CONS_Printf("%d/%d sectors, %d/%d lines saved\n", statsec, numsectors, statline, numlines);

  //----------------------------------------------
  // polyobjs
  a.Marker(MARK_POLYOBJ);

  for (i = 0; i < NumPolyobjs; i++)
    a << polyobjs[i].tag << polyobjs[i].angle
      << polyobjs[i].spawnspot.x << polyobjs[i].spawnspot.y;

  //----------------------------------------------
  // scripts
  a.Marker(MARK_SCRIPT);

  for (i = 0; i < ACScriptCount; i++)
    {
      a << (int &)ACSInfo[i].state;
      a << ACSInfo[i].waitValue;
    }
  a.Write((byte *)ACMapVars, sizeof(ACMapVars));

  // FS: levelscript contains the map global variables (everything else can be loaded from the WAD)

  // count number of variables
  n = 0;
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript->variables[i];
      while (sv && sv->type != svt_label)
        {
          n++;
          sv = sv->next;
        }
    }
  a << n;

  // go thru hash chains, store each variable
  for (i=0; i<VARIABLESLOTS; i++)
    {
      // go thru this hashchain
      svariable_t *sv = levelscript->variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while (sv && sv->type != svt_label)
        {
	  sv->Serialize(a);
          sv = sv->next;
        }
    }

  //runningscripts (scripts currently suspended)
  runningscript_t *rs;
  
  // count runningscripts
  n = 0;
  for (rs = runningscripts; rs; rs = rs->next)
    n++;
  a << n;
  
  // now archive them
  for (rs = runningscripts; rs; rs = rs->next)
    {
      a << rs->script->scriptnum;
      a << (n = (rs->savepoint - rs->script->data)); // offset
      a << (int &)rs->wait_type;
      a << rs->wait_data;

      Thinker::Serialize(rs->trigger, a);
  
      // count number of variables
      n = 0;
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
	  while (sv && sv->type != svt_label)
	    {
	      n++;
	      sv = sv->next;
	    }
	}
      a << n;

      // go thru hash chains, store each variable
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
	  while (sv && sv->type != svt_label)
	    {
	      sv->Serialize(a);
	      sv = sv->next;
	    }
	}
    }

  // TODO Archive the script camera.

  //----------------------------------------------
  // Thinkers (including map objects aka Actors)
  a.Marker(MARK_THINK);

  Thinker *th;
  // Now we use a one-pass recursive iteration of thinkers.
  //   Another possible way to do this:
  //   First do a recursive iteration of the thinkers, ONLY to create the pointer->id map.
  //   Then iterate the map serializing each element in turn (first writing the ID, of course).

  i = 0;
  // only the stuff in the thinker ring is checked
  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      th->CheckPointers(); // clear pointers to deleted items
      i++;
    }

  a << i; // store the number of thinkers in ring

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    Thinker::Serialize(th, a);

  a.Marker(MARK_MISC);
  //----------------------------------------------
  // respawnqueue
  n = itemrespawnqueue.size();
  a << n;
  for (i=0; i<n; i++)
    {
      temp = itemrespawnqueue[i] - mapthings;
      a << temp;
      a << itemrespawntime[i];
    }

  //----------------------------------------------
  // the rest
  multimap<short, Actor *>::iterator t;
  n = TIDmap.size();
  a << n;
  for (t = TIDmap.begin(); t != TIDmap.end(); t++)
    {
      stemp = t->first;
      a << stemp;
      if (a.HasStored(t->second, temp))
	a << temp;
      else
	I_Error("Crap in TIDmap!\n");
    }

  // TODO Sound sequences

  return 0;
}


int Map::Unserialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, n;

  if (!a.Marker(MARK_MAP))
    return -1;

  a.active_map = this; // so the Thinkers can be extracted OK

  // first we load and setup the map, but without spawning any Thinkers if possible
  a << lumpname;
  // TODO load map md5 checksum, make sure the correct map is loaded
  Setup(0, false);
  // remove all the current thinkers (could be done more elegantly by not spawning them
  // in the first place, but... well, at least the Actors are not spawned.)
  Thinker *th, *next;
  for (th = thinkercap.next; th != &thinkercap; th = next)
    {
      next = th->next;
      delete th;
    }
  InitThinkers();

  a << starttic << maptic;
  a << kills << items << secrets;

  line_t *li;
  side_t *si;
  byte    diff, diff2;

  // load changes in geometry
  while (1)
    {
      a << temp;
      if (temp == MARK_END)
	break;
      i = temp; // sector number

      a << diff;
      if (diff & SD_DIFF2)
	a << diff2;
      else
	diff2 = 0;

      char picname[8];

      if (diff & SD_FLOORHT ) a << sectors[i].floorheight;
      if (diff & SD_CEILHT  ) a << sectors[i].ceilingheight;
      if (diff & SD_FLOORPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].floorpic = tc.GetID(picname, TEX_floor);
        }
      if (diff & SD_CEILPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].ceilingpic = tc.GetID(picname, TEX_floor);
        }
      if (diff & SD_LIGHT)    a << sectors[i].lightlevel;
      if (diff & SD_SPECIAL)  a << sectors[i].special;
      if (diff & SD_TAG)      a << sectors[i].tag;

      if (diff2 & SD_FXOFFS)  a << sectors[i].floor_xoffs;
      if (diff2 & SD_FYOFFS)  a << sectors[i].floor_yoffs;
      if (diff2 & SD_CXOFFS)  a << sectors[i].ceiling_xoffs;
      if (diff2 & SD_CYOFFS)  a << sectors[i].ceiling_yoffs;

      if (diff2 & SD_STAIRLOCK) a << sectors[i].stairlock;
      else sectors[i].stairlock = 0;
      if (diff2 & SD_NEXTSEC)   a << sectors[i].nextsec;
      else sectors[i].nextsec = -1;
      if (diff2 & SD_PREVSEC)   a << sectors[i].prevsec;
      else sectors[i].prevsec = -1;

      if (diff2 & SD_SEQTYPE)   a << sectors[i].seqType;
    }

  while (1)
    {
      a << temp; // line number
      if (temp == MARK_END)
	break;
      li = &lines[temp];

      a << diff;

      if (diff & LD_DIFF2) a << diff2;
      else diff2 = 0;
      if (diff & LD_FLAG)    a << li->flags;
      if (diff & LD_SPECIAL) a << li->special;
      if (diff & LD_ARGS)
	for (i=0; i<5; i++)
	  a << li->args[i];

      si = li->sideptr[0];
      if (diff & LD_S1TEXOFF) a << si->textureoffset;
      if (diff & LD_S1TOPTEX) a << si->toptexture;
      if (diff & LD_S1BOTTEX) a << si->bottomtexture;
      if (diff & LD_S1MIDTEX) a << si->midtexture;

      si = li->sideptr[1];
      if (diff2 & LD_S2TEXOFF) a << si->textureoffset;
      if (diff2 & LD_S2TOPTEX) a << si->toptexture;
      if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
      if (diff2 & LD_S2MIDTEX) a << si->midtexture;
    }

  //----------------------------------------------
  if (!a.Marker(MARK_POLYOBJ))
    return -2;

  for (i = 0; i < NumPolyobjs; i++)
    {
      a << n;
      if (n != polyobjs[i].tag)
	I_Error("Invalid polyobj tag!\n");

      angle_t ang;
      a << ang;
      PO_RotatePolyobj(&polyobjs[i], ang);
      fixed_t x, y;
      a << x << y;
      PO_MovePolyobj(&polyobjs[i], x - polyobjs[i].spawnspot.x,
		     y - polyobjs[i].spawnspot.y);
    }

  //----------------------------------------------
  // scripts
  if (!a.Marker(MARK_SCRIPT))
    return -3;

  for (i = 0; i < ACScriptCount; i++)
    {
      a << n; ACSInfo[i].state = acs_state_t(n);
      a << ACSInfo[i].waitValue;
    }
  a.Read((byte *)ACMapVars, sizeof(ACMapVars));

  // FS: restore levelscript
  // free all the variables in the current levelscript first
  
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript->variables[i];
      
      while (sv && sv->type != svt_label)
        {
          svariable_t *next = sv->next;
          Z_Free(sv);
          sv = next;
        }
      levelscript->variables[i] = sv;       // null or label
    }

  // now read the number of variables from the savegame file
  a << n;
  for (i=0; i<n; i++)
    {
      svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
      sv->Unserialize(a);
      
      // link in the new variable
      int hashkey = script_t::variable_hash(sv->name);
      sv->next = levelscript->variables[hashkey];
      levelscript->variables[hashkey] = sv;
    }

  // restore runningscripts
  // remove all runningscripts first: levelscript may have started them
  FS_ClearRunningScripts(); 

  a << n;
  for (i=0; i<n; i++)
    {
      // create a new runningscript
      runningscript_t *rs = new runningscript_t();
  
      int scriptnum;
      a << scriptnum;
    
      // levelscript?
      if (scriptnum == -1)
	rs->script = levelscript;
      else
	rs->script = levelscript->children[scriptnum];

      a << n; // read out offset from save
      rs->savepoint = rs->script->data + n;
      a << n;
      rs->wait_type = fs_wait_e(n);
      a << rs->wait_data;
      rs->trigger = (Actor *)Thinker::Unserialize(a);
    
      // read out the variables now (fun!)
      // start with basic script slots/labels
  
      for (i=0; i<VARIABLESLOTS; i++)
	rs->variables[i] = rs->script->variables[i];

      // get number of variables
      a << n;
      for (i=0; i<n; i++)
	{
	  svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
	  sv->Unserialize(a);

	  // link in the new variable
	  int hashkey = script_t::variable_hash(sv->name);
	  sv->next = rs->variables[hashkey];
	  rs->variables[hashkey] = sv;
	}
      
      // hook into chain
      FS_AddRunningScript(rs);
    }

  // TODO Unarchive the script camera

  //----------------------------------------------
  // Thinkers
  if (!a.Marker(MARK_THINK))
    return -4;

  a << n;
  for (i=0; i<n; i++)
    {
      Thinker *th = Thinker::Unserialize(a);
      AddThinker(th);
    }

  if (!a.Marker(MARK_MISC))
    return -5;
  //----------------------------------------------
  // respawnqueue
  a << n;
  for (i=0; i<n; i++)
    {
      a << temp;
      itemrespawnqueue.push_back(&mapthings[temp]);
      a << temp;
      itemrespawntime.push_back(temp);
    }

  //----------------------------------------------
  // the rest
  TIDmap.clear();
  a << n;
  for (i=0; i<n; i++)
    {
      Actor *p = NULL;
      a << stemp << temp;
      if (a.GetPtr((int &)temp, (void * &)p))
	TIDmap.insert(pair<const short, Actor*>(stemp, p));
      else
	I_Error("Crap in TIDmap!\n");
    }


  return 0;
}



int MapCluster::Serialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << time << partime;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n;
  a << (n = maps.size());
  for (int i=0; i<n; i++)
    a << maps[i]->mapnumber;
  return 0;
}


int MapCluster::Unserialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << time << partime;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n, temp;
  a << n;
  for (int i=0; i<n; i++)
    {
      a << temp;
      maps.push_back(game.FindMapInfo(temp));
    }
  return 0;
}


int MapInfo::Serialize(LArchive &a)
{
  int temp;
  a << (int &)state;
  a << found;

  a << lumpname << nicename << savename;
  a << cluster << mapnumber;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime;
  a << musiclump;

  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  a << interpic << intermusic;

  if (state == MAP_SAVED)
    {
      // utilize the existing hubsave file
      a << (temp = 2);
      byte *buffer;
      int length = FIL_ReadFile(savename.c_str(), &buffer);
      if (!length)
	{
	  I_Error("Couldn't read hubsave file %s", savename.c_str());
	  return -1;
	}

      a << length;
      a.Write(buffer, length);
      Z_Free(buffer);
    }
  else if (me)
    {
      a << (temp = 1);
      me->Serialize(a);
    }
  else
    a << (temp = 0);

  return 0;
}


int MapInfo::Unserialize(LArchive &a)
{
  int temp;
  a << temp; state = mapstate_e(temp);
  a << found;

  a << lumpname << nicename << savename;
  a << cluster << mapnumber;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime;
  a << musiclump;

  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  a << interpic << intermusic;

  a << temp;
  if (temp == 2)
    {
      // extract the hubsave file
      int length;
      a << length;
      byte *buffer = (byte *)Z_Malloc(length, PU_STATIC, NULL);
      a.Read(buffer, length);
      FIL_WriteFile(savename.c_str(), buffer, length);
      Z_Free(buffer);
    }
  else if (temp == 1)
    {
      me = new Map(this);
      me->Unserialize(a);
      a.active_map = NULL; // this map is done
    }
  else
    me = NULL;

  return 0;
}



//==============================================
//  Player serialization
//==============================================

int PlayerInfo::Serialize(LArchive &a)
{
  int n;
  a << number << team << name;
  a << client_hash;

  a << (int &)playerstate;
  a << spectator;

  a << requestmap << entrypoint;

  // cmd can be ignored

  // scoring
  a << (n = Frags.size());
  map<int, int>::iterator t;
  for (t = Frags.begin(); t != Frags.end(); t++)
    {
      int m = t->first;
      int n = t->second;
      a << m << n;
    }
  a << score << kills << items << secrets << time;

  options.Serialize(a);

  // current feedback is lost

  // mp is handled through the pawn
  // players are serialized after maps, so pawn may already be stored
  Thinker::Serialize(pawn, a); 
  Thinker::Serialize(pov, a); 
  Thinker::Serialize(hubsavepawn, a); 

  return 0;
}

int PlayerInfo::Unserialize(LArchive &a)
{
  int i, n;
  a << number << team << name;
  a << client_hash; // so we can recognize the clients after loading

  a << n; playerstate = playerstate_t(n);
  a << spectator;

  a << requestmap << entrypoint;

  a << n;
  for (i=0; i<n; i++)
    {
      int t1, t2;
      a << t1 << t2;
      Frags.insert(pair<int, int>(t1, t2));
    }
  a << score << kills << items << secrets << time;

  options.Unserialize(a);

  pawn = static_cast<PlayerPawn*>(Thinker::Unserialize(a));
  pov = static_cast<Actor*>(Thinker::Unserialize(a));
  hubsavepawn = static_cast<PlayerPawn*>(Thinker::Unserialize(a));

  if (pawn)
    {
      mp = pawn->mp;
      pawn->player = this;
    }

  return 0;
}


int PlayerOptions::Serialize(LArchive &a)
{
  a << ptype << pclass << color << skin;

  a << autoaim << originalweaponswitch;
  for (int i=0; i<NUMWEAPONS; i++)
    a << weaponpref[i];

  a << messagefilter;
  return 0;
}

int PlayerOptions::Unserialize(LArchive &a)
{
  a << ptype << pclass << color << skin;

  a << autoaim << originalweaponswitch;
  for (int i=0; i<NUMWEAPONS; i++)
    a << weaponpref[i];

  a << messagefilter;
  return 0;
}


//==============================================
//  Game serialization
//==============================================

int TeamInfo::Serialize(LArchive &a)
{
  a << name << color << score << resources;
  return 0;
}

int TeamInfo::Unserialize(LArchive &a)
{
  a << name << color << score << resources;
  return 0;
}


// takes a snapshot of the entire game state and stores it in the archive
int GameInfo::Serialize(LArchive &a)
{
  int i, n;

  // treat all enums as ints
  a << demoversion;
  a << (int &)mode;
  a << (int &)state;
  a << (int &)skill;

  // flags
  a << netgame << multiplayer << modified << paused << inventory;

  a << maxteams;
  a << maxplayers;

  a.Marker(MARK_GROUP);

  // mapinfo (and maps)
  a << (n = mapinfo.size());
  mapinfo_iter_t k;
  for (k = mapinfo.begin(); k != mapinfo.end(); k++)
    k->second->Serialize(a);

  a.Marker(MARK_GROUP);

  // clustermap
  a << (n = clustermap.size());
  cluster_iter_t l;
  for (l = clustermap.begin(); l != clustermap.end(); l++)
    l->second->Serialize(a);

  a << currentcluster->number;

  a.Marker(MARK_GROUP);

  // teams
  a << (n = teams.size());
  for (i = 0; i < n; i++)
    teams[i]->Serialize(a);

  a.Marker(MARK_GROUP);

  // players 
  a << (n = Players.size());
  player_iter_t j;
  for (j = Players.begin(); j != Players.end(); j++)
    j->second->Serialize(a);

  a.Marker(MARK_GROUP);

  // global script data
  a.Write((byte *)WorldVars, sizeof(WorldVars));
  acsstore_iter_t t;
  a << (n = ACS_store.size());
  for (t = ACS_store.begin(); t != ACS_store.end(); t++) 
    a.Write((byte *)&t->second, sizeof(acsstore_t));

  // TODO FS hub_script, global_script...

  a.Marker(MARK_GROUP);
  {
    // client stuff
    int temp = NUM_LOCALPLAYERS;
    a << temp;
    for (i = 0; i < temp; i++)
      a << (n = LocalPlayers[i].info ? LocalPlayers[i].info->number : -1);

    // TODO how to save net info???
  }

  a.Marker(MARK_GROUP);
  {
    a << (n = P_GetRandIndex());

    const char *temp = S.GetMusic();
    string mus;
    if (temp)
      mus = temp;
    else
      mus = "";
    a << mus;
  }

  //CV_SaveNetVars((char**)&save_p);
  //CV_LoadNetVars((char**)&save_p);

  // TODO
  // client info and other net stuff (player authentication?)
  // consvars
  // check if required resource files are to be found / can be downloaded

  return 0;
}


int GameInfo::Unserialize(LArchive &a)
{
  int i, n;
  // treat all enums as ints
  a << demoversion;
  a << n; mode = gamemode_t(n);
  a << n; state = gamestate_t(n);
  a << n; skill = skill_t(n);

  // flags
  a << netgame << multiplayer << modified << paused << inventory;

  a << maxteams;
  a << maxplayers;

  if (!a.Marker(MARK_GROUP))
    return -1;

  // mapinfo (and maps)
  a << n;
  for (i = 0; i < n; i++)
    {
      MapInfo *m = new MapInfo;
      if (m->Unserialize(a))
	return -2;
      mapinfo[m->mapnumber] = m;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // clustermap
  a << n;
  for (i = 0; i < n; i++)
    {
      MapCluster *c = new MapCluster;
      if (c->Unserialize(a))
	return -3;
      clustermap[c->number] = c;
    }

  a << n;
  currentcluster = clustermap[n];

  if (!a.Marker(MARK_GROUP))
    return -1;

  // teams
  a << n;
  teams.resize(n);
  for (i = 0; i < n; i++)
    {
      teams[i] = new TeamInfo;
      if (teams[i]->Unserialize(a))
	return -4;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // players 
  a << n;
  for (i = 0; i < n; i++)
    {
      PlayerInfo *p = new PlayerInfo;
      if (p->Unserialize(a))
	return -5;
      Players[p->number] = p;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // global script data
  a.Read((byte *)WorldVars, sizeof(WorldVars));
  a << n;
  for (i = 0; i < n; i++)
    {
      acsstore_t temp;
      a.Read((byte *)&temp, sizeof(acsstore_t));
      ACS_store.insert(pair<const int, acsstore_t>(temp.tmap, temp));
    }

  if (!a.Marker(MARK_GROUP))
    return -1;

  // client stuff
  a << n;
  for (i = 0; i < n; i++)
    {
      int num;
      a << num;
      if (num == -1)
	LocalPlayers[i].info = NULL;
      else
	LocalPlayers[i].info = FindPlayer(num);
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  // misc shit

  a << n;
  P_SetRandIndex(n);

  string mus;
  a << mus;
  S.StartMusic(mus.c_str(), true);

  return 0;
}



void GameInfo::LoadGame(int slot)
{
  CONS_Printf("Loading a game...\n");
  char  savename[255];
  byte *savebuffer;

  sprintf(savename, savegamename, slot);

  int length = FIL_ReadFile(savename, &savebuffer);
  if (!length)
    {
      CONS_Printf("Couldn't open save file %s\n", savename);
      return;
    }

  LArchive a;
  if (!a.Open(savebuffer, length))
    return;

  Z_Free(savebuffer); // the compressed buffer is no longer needed

  Downgrade(LEGACY_VERSION); // reset the game version
  SV_SpawnServer(-1);

  // dearchive all the modifications
  if (Unserialize(a))
    {
      CONS_Printf("\3Savegame file corrupted!\n\n");
      SV_Reset();
      return;
    }

  state = GS_LEVEL;

  if (netgame)
    net->SV_Open(true); // let the remote players in

  paused = false;

  // view the local human players by default
  for (int i=0; i < NUM_LOCALHUMANS; i++)
    if (LocalPlayers[i].info)
      ViewPlayers.push_back(LocalPlayers[i].info);

  // TODO have other playerinfos waiting for clients to rejoin
  if (ViewPlayers.size())
    hud.ST_Start(ViewPlayers[0]);
  // done
  /*
  if (setsizeneeded)
    R_ExecuteSetViewSize();

  R_FillBackScreen();  // draw the pattern into the back screen
  */
  con.Toggle(true);
  CONS_Printf("...done.\n");
}


void GameInfo::SaveGame(int savegameslot, char *description)
{
  CONS_Printf("Saving the game...\n");

  LArchive a;
  a.Create(description); // create a new save archive
  Serialize(a);          // store the game state into it

  byte *buffer;
  unsigned length = a.Compress(&buffer, 1);  // take out the compressed data

  char filename[256];
  sprintf(filename, savegamename, savegameslot);

  CONS_Printf("Savegame: %d bytes\n", length);
  FIL_WriteFile(filename, buffer, length);

  Z_Free(buffer);

  CONS_Printf("...done.\n");
  //R_FillBackScreen();  // draw the pattern into the back screen
}
