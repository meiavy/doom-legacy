// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
//
// Revision 1.31  2002/09/27 08:18:41  vberghol
// intermission fixed.
//
// Revision 1.30  2002/09/25 15:17:37  vberghol
// Intermission fixed?
//
// Revision 1.26  2002/09/05 14:12:13  vberghol
// network code partly bypassed
//
// Revision 1.23  2002/08/30 11:45:40  vberghol
// players system modified
//
// Revision 1.22  2002/08/27 11:51:46  vberghol
// Menu rewritten
//
// Revision 1.21  2002/08/24 12:39:36  vberghol
// lousy fix
//
// Revision 1.20  2002/08/21 16:58:30  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.19  2002/08/20 17:01:45  vberghol
// Now it compiles. Will it link? Or work?
//
// Revision 1.18  2002/08/20 13:56:57  vberghol
// sdfgsd
//
// Revision 1.17  2002/08/19 18:06:38  vberghol
// renderer somewhat fixed
//
// Revision 1.16  2002/08/17 21:21:44  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.15  2002/08/17 16:02:03  vberghol
// final compile for engine!
//
// Revision 1.14  2002/08/16 20:49:24  vberghol
// engine ALMOST done!
//
// Revision 1.13  2002/08/13 19:47:40  vberghol
// p_inter.cpp done
//
// Revision 1.12  2002/08/11 17:16:47  vberghol
// ...
//
// Revision 1.11  2002/08/06 13:14:20  vberghol
// ...
//
// Revision 1.10  2002/08/02 20:14:49  vberghol
// p_enemy.cpp done!
//
// Revision 1.9  2002/07/23 19:21:39  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.8  2002/07/18 19:16:36  vberghol
// renamed a few files
//
// Revision 1.7  2002/07/16 19:16:19  vberghol
// Hardware sound interface again somewhat fixed
//
// Revision 1.6  2002/07/15 20:52:38  vberghol
// w_wad.cpp (FileCache class) finally fixed
//
// Revision 1.5  2002/07/04 18:02:25  vberghol
// Pient� fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.4  2002/07/01 21:00:14  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:53  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.3  2000/03/29 19:39:48  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    Part of GameInfo class implementation. Methods related to game state changes.
//-----------------------------------------------------------------------------


#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_level.h"
#include "g_pawn.h"

#include "g_input.h" // gamekeydown!
#include "m_misc.h" // FIL_* file functions
#include "dstrings.h"
#include "m_random.h"
#include "m_menu.h"
#include "am_map.h"

#include "t_script.h"
#include "t_func.h"

#include "s_sound.h"
#include "sounds.h"

#include "f_finale.h"
#include "d_netcmd.h"
#include "d_clisrv.h"
#include "hu_stuff.h" // HUD
#include "p_camera.h" // camera
#include "wi_stuff.h"
#include "console.h"
#include "z_zone.h"


bool         nomusic;    
bool         nosound;
language_t   language = english;            // Language.

GameInfo game;


// This function recreates the classical Doom/DoomII/Heretic maplist
// using episode and game.mode info
// Most of the original game dependent crap is here,
// all the other code is general and clean.

#include "sounds.h"

LevelNode *G_CreateClassicMapList(int episode)
{
  const char *HereticSky[5] = {"SKY1", "SKY2", "SKY3", "SKY1", "SKY3"};

  const int DoomBossKey[4] = { 1, 2, 8, 16 };
  const int HereticBossKey[5] = { 0x200, 0x800, 0x1000, 0x400, 0x800 };

  const int DoomSecret[4] = { 3, 5, 6, 2 };
  const int HereticSecret[5] = { 6, 4, 4, 4, 3 };
  const int DoomPars[4][9] =
  {
    {30,75,120,90,165,180,180,30,165},
    {90,90,90,120,90,360,240,30,170},
    {90,45,90,150,90,90,165,30,135},
    {0}
  };
  const int HereticPars[5][9] =
  {
    {90,0,0,0,0,0,0,0,0},
    {0},
    {0},
    {0},
    {0}
  };
  const int DoomIIPars[32] =
  {
    30,90,120,120,90,150,120,120,270,90,        //  1-10
    210,150,150,150,210,150,420,150,210,150,    // 11-20
    240,150,180,150,150,300,330,420,300,180,    // 21-30
    120,30                                      // 31-32
  };
  // FIXME! check the partimes! Heretic and Doom I episode 4!
  // I made these Heretic partimes by myself...
  // finale flat names
  const char DoomFlat[4][9] = {"FLOOR4_8", "SFLR6_1", "MFLR8_4", "MFLR8_3"};
  const char HereticFlat[5][9] = {"FLOOR25", "FLATHUH1", "FLTWAWA2", "FLOOR28", "FLOOR08"};

  int i, base, base2;
  char name[9];
  LevelNode *m;
    
  switch (game.mode)
    {
    case commercial:
      base2 = C1TEXT_NUM;
      switch (game.mission)
	{
	case gmi_tnt:
	  base = THUSTR_1_NUM;
	  base2 = T1TEXT_NUM;
	  break;
	case gmi_plut:
	  base = PHUSTR_1_NUM;
	  break;
	default:
	  base = HUSTR_1_NUM;
	}
      m = new LevelNode[32];
      for (i=0; i<32; i++)
	{
	  sprintf(name, "MAP%2.2d", i+1);
	  m[i].mapname = name;
	  m[i].levelname = text[base + i];
	  m[i].exit.push_back(m+i+1); // next level
	  m[i].partime = DoomIIPars[i];
	  m[i].interpic = "INTERPIC";
	  if (i < 11)
	    m[i].skyname = "SKY1";
	  else if (i < 20)
	    m[i].skyname = "SKY2";
	  else
	    m[i].skyname = "SKY3";
	  m[i].musicname = MusicNames[mus_runnin + i];
	}
      m[29].exit[0] = NULL; // finish
      m[30].exit[0] = &m[15]; // return from secret
      m[31].exit[0] = &m[15]; // return from ss

      m[14].exit.push_back(&m[30]); // secret
      m[30].exit.push_back(&m[31]); // super secret

      m[6].BossDeathKey = 32+64; // fatsos and baby spiders
      m[29].BossDeathKey = 256;  // brain
      m[31].BossDeathKey = 128;  // keen

      m[5].finaletext = text[base2];
      m[5].finaleflat = "SLIME16";
      m[10].finaletext = text[base2+1];
      m[10].finaleflat = "RROCK14";
      m[19].finaletext = text[base2+2];
      m[19].finaleflat = "RROCK07";
      m[29].finaletext = text[base2+3];
      m[29].finaleflat = "RROCK17";
      m[14].finaletext = text[base2+4];
      m[14].finaleflat = "RROCK13";
      m[30].finaletext = text[base2+5];
      m[30].finaleflat = "RROCK19";
      break;

    case shareware:
      if (episode != 1) return NULL;
    case registered:
      if (episode < 1 || episode > 3) return NULL;
    case retail:
      if (episode < 1 || episode > 4) return NULL;

      base = HUSTR_E1M1_NUM;
      m = new LevelNode[9];
      for (i=0; i<9; i++)
	{
	  m[i].episode = episode;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  m[i].mapname = name;
	  m[i].levelname = text[base + (episode-1)*9 + i];
	  m[i].exit.push_back(m+i+1);
	  m[i].partime = DoomPars[episode-1][i];
	  sprintf(name, "WIMAP%d", (episode-1)%3);
	  m[i].interpic = name;
	  m[i].skyname = string("SKY") + char('0' + episode);	  
	  m[i].musicname = MusicNames[mus_e1m1 + (episode-1)*9 + i];
	}
      m[7].exit[0] = NULL;
      m[8].exit[0] = &m[DoomSecret[episode-1]];

      m[DoomSecret[episode-1] - 1].exit.push_back(&m[8]); // secret
      m[7].BossDeathKey = DoomBossKey[episode-1];
      if (episode == 4) m[5].BossDeathKey = 4; // cyborg in map 6...

      m[7].finaletext = text[E1TEXT_NUM + episode-1];
      m[7].finaleflat = DoomFlat[episode-1];
      break;

    case heretic:
      base = HERETIC_E1M1_NUM;
      m = new LevelNode[9];
      for (i=0; i<9; i++)
	{
	  m[i].episode = episode;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  m[i].mapname = name;
	  m[i].levelname = text[base + (episode-1)*9 + i];
	  m[i].exit.push_back(m+i+1);
	  m[i].partime = HereticPars[episode-1][i];
	  sprintf(name, "MAPE%d", ((episode-1)%3)+1);
	  m[i].interpic = name;
	  m[i].skyname = HereticSky[episode-1];
	  m[i].musicname = MusicNames[mus_he1m1 + (episode-1)*9 + i];
	}
      m[7].exit[0] = NULL;
      m[8].exit[0] = &m[HereticSecret[episode-1]];

      m[HereticSecret[episode-1] - 1].exit.push_back(&m[8]); //secret
      m[7].BossDeathKey = HereticBossKey[episode-1];

      m[7].finaletext = text[HERETIC_E1TEXT + episode-1];
      m[7].finaleflat = HereticFlat[episode-1];
      break;

    default:
      m = NULL;
    }
  return m;
}
/*
// this is useless now since we have team numbers for each player?
bool GameInfo::SameTeam(int a, int b)
{
  extern consvar_t cv_teamplay;

  switch (cv_teamplay.value)
    {
    case 0:
      return false;
    default:
      return (players[a]->team == players[b]->team);
    }
}
*/

void GameInfo::UpdateScore(PlayerInfo *killer, PlayerInfo *victim)
{
  killer->frags[victim->number]++;
  
  // scoring rule
  if (cv_teamplay.value == 0)
    {
      if (killer != victim)
	killer->score++;
      else
	killer->score--;
    }
  else
    {
      if (killer->team != victim->team)
	{
	  teams[killer->team]->score++;
	  killer->score++;
	}
      else
	{
	  teams[killer->team]->score--;
	  killer->score--;
	}
    }

  // check fraglimit cvar
  if (cv_fraglimit.value)
    killer->CheckFragLimit();
}

int GameInfo::GetFrags(fragsort_t **fragtab, int type)
{
  // PlayerInfo class holds a raw frags table for each player.
  // It also holds a cumulative, game type sensitive frag field named score
  // if the game type is changed in the middle of a game, score is NOT zeroed,
  // just the scoring system is different from there on.

  // scoring is done in PlayerPawn::Kill

  extern consvar_t cv_teamplay;
  int i, j;
  int n = players.size();
  int m = teams.size();
  int ret;
  fragsort_t *ft;
  int **teamfrags;

  if (cv_teamplay.value)
    {
      ft = new fragsort_t[m];
      teamfrags = (int **)(new int[m][m]);

      for (i=0; i<m; i++)
	{
	  ft[i].num   = i; // team 0 are the unteamed
	  ft[i].color = teams[i]->color;
	  ft[i].name  = teams[i]->name.c_str();
	}

    // calculate teamfrags
    int team1, team2;
    for (i=0; i<n; i++)
      {
	team1 = players[i]->team;

	for (j=0; j<n; j++)
	  {
	    team2 = players[j]->team;

	    teamfrags[team1][team2] +=
	      players[i]->frags[players[j]->number - 1];
	  }
      }

    // type is a magic number telling which fragtable we want
    switch (type)
      {
      case 0: // just normal frags
	for (i=0; i<m; i++)
	  ft[i].count = teams[i]->score;
	break;

      case 1: // buchholtz
	for (i=0; i<m; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<m; j++)
	      if (i != j)
		ft[i].count += teamfrags[i][j] * teams[j]->score;
	  }
	break;

      case 2: // individual
	for (i=0; i<m; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<m; j++)
	      if (i != j)
		{
		  if(teamfrags[i][j] > teamfrags[j][i])
		    ft[i].count += 3;
		  else if(teamfrags[i][j] == teamfrags[j][i])
		    ft[i].count += 1;
		}
        }
      break;

      case 3: // deaths
	for (i=0; i<m; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<m; j++)
	      ft[i].count += teamfrags[j][i];
	  }
	break;
      
      default:
	break;
      }
    delete [] teamfrags;
    ret = m;

  } else { // not teamgame
    ft = new fragsort_t[n]; 

    for (i=0; i<n; i++)
      {
	ft[i].num = players[i]->number;
	ft[i].color = players[i]->pawn->color;
	ft[i].name  = players[i]->name.c_str();
      }


    // type is a magic number telling which fragtable we want
    switch (type)
      {
      case 0: // just normal frags
	for (i=0; i<n; i++)
	  ft[i].count = players[i]->score;
	break;

      case 1: // buchholtz
	for (i=0; i<n; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<n; j++)
	      if (i != j)
		{
		  int k = players[j]->number - 1;
		  ft[i].count += players[i]->frags[k]*(players[j]->score + players[j]->frags[k]);
		  // FIXME is this formula correct?
		}
	  }
	break;

      case 2: // individual
	for (i=0; i<n; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<n; j++)
	      if (i != j)
		{
		  int k = players[i]->number - 1;
		  int l = players[j]->number - 1;
		  if(players[i]->frags[l] > players[j]->frags[k])
		    ft[i].count += 3;
		  else if(players[i]->frags[l] == players[j]->frags[k])
		    ft[i].count += 1;
		}
	  }
	break;

      case 3: // deaths
	for (i=0; i<n; i++)
	  {
	    ft[i].count = 0;
	    for (j=0; j<n; j++)
	      ft[i].count += players[j]->frags[players[i]->number - 1];
	  }
	break;
      
      default:
	break;
      }
    ret = n;
  }

  *fragtab = ft; 
  return ret;
}


// Tries to add a player into the game. The new player gets the number pnum+1
// if it is free, otherwise she gets the next free number.
// An already constructed PI can be given in "in", if necessary.
// Returns NULL if a new player cannot be added.

PlayerInfo *GameInfo::AddPlayer(int pnum, PlayerInfo *in = NULL)
{
  // a negative pnum just uses the first free slot
  int n = players.size();

  // no room in game
  if (n >= maxplayers)
    return NULL;

  // too high a pnum
  if (pnum >= maxplayers)
    pnum = -1;

  //vector<bool> present(maxplayers, false);
  /*
    // not needed anymore because now "present" vector is available at GameInfo
  for (i=0; i<n; i++)
    {
      // what if maxplayers has recently been set to a lower-than-n value?
      // when are the extra players kicked?
      present[players[i]->number-1] = true;
    }
  */

  if (pnum >= present.size())
    present.resize(maxplayers);
  else if (pnum >= 0 && present[pnum])
  // pnum already taken
    pnum = -1;

  int m = present.size();
  int i;
  // find first free player number, if necessary
  if (pnum < 0)
    {
      for (i=0; i<m; i++)
	if (present[i] == NULL)
	  {
	    pnum = i;
	    break;
	  }
      // make room for one more player
      if (i == m)
	{
	  present.push_back(NULL);
	  pnum = i;
	}
    }

  // player number is not too high, has not been taken and there's room in the game!

  PlayerInfo *p;

  // if a valid PI is given, use it. Otherwise, create a new PI.
  if (in == NULL)
    p = new PlayerInfo();
  else
    p = in;

  p->number = pnum+1;
  players.push_back(p);
  present[pnum] = p;

  // FIXME the frags vectors of other players must be lengthened

  return p;

  /*
    p = &players[playernum];
    memset(p->inventory, 0, sizeof(p->inventory));
    p->inventorySlotNum = 0;
    p->inv_ptr = 0;
    p->st_curpos = 0;
    p->st_inventoryTics = 0;

  if (game.mode == heretic)
    p->weaponinfo = wpnlev1info;
  else
    p->weaponinfo = doomweaponinfo;
  */
}


// Removes a player from game.
// This and ClearPlayers are the ONLY ways a player should be removed.
void GameInfo::RemovePlayer(vector<PlayerInfo *>::iterator it)
{
  // it is an iterator to _players_ vector ! NOT a player number!
  int j, n = players.size();

  if (it >= players.end())
    return;

  PlayerInfo *p = *it;

  for (j = 0 ; j<n ; j++)
    {
      // the frags vector is not compacted (shortened) now, only between levels 
      players[j]->frags[p->number - 1] = 0;
    }
	    
  // remove avatar of player
  if (p->pawn)
    {
      p->pawn->player = NULL;
      p->pawn->Remove();
    }

  // FIXME make sure that global PI pointers are still OK
  if (consoleplayer == p) consoleplayer = NULL;
  if (consoleplayer2 == p) consoleplayer2 = NULL;

  if (displayplayer == p) displayplayer = NULL;
  if (displayplayer2 == p) displayplayer2 = NULL;

  present[p->number - 1] = NULL;
  delete p;
  // NOTE! because PI's are deleted, even local PI's must be dynamically
  // allocated, using a copy constructor: new PlayerInfo(localplayer).
  players.erase(it);
}

// Removes all players from a game.
void GameInfo::ClearPlayers()
{
  int i, n = players.size();
  PlayerInfo *p;

  for (i=0; i<n; i++)
    {
      p = players[i];
      // remove avatar of player
      if (p->pawn)
	{
	  p->pawn->player = NULL;
	  p->pawn->Remove();
	}
      delete p;
    }
  present.clear();
  players.clear();
  consoleplayer = consoleplayer2 = NULL;
  displayplayer = displayplayer2 = NULL;
}

void F_Ticker();
void D_PageTicker();
//
// was G_Ticker
// Make ticcmd_ts for the players.
//
void GameInfo::Ticker()
{
  extern ticcmd_t netcmds[32][32];
  extern bool dedicated;
  int i;
  int n = players.size();

  // level is physically exited -> ExitLevel(int exit)
  // ExitLevel(int exit): action is set to ga_completed
  // 
  // LevelCompleted(): end level, start intermission (set state), reset action to ga_nothing
  // intermission ends -> EndIntermission()
  // EndIntermission(): check winning/finale (set state, init finale), else set ga_worlddone
  //
  // WorldDone(): action to ga_nothing, load next level, set state to GS_LEVEL

  // do things to change the game state
  while (action != ga_nothing)
    switch (action)
      {
      case ga_completed :
	LevelCompleted();
	break;
      case ga_worlddone :
	WorldDone();
	break;
      case ga_nothing   :
	break;
      default : I_Error("game.action = %d\n", action);
      }

  CONS_Printf("======== GI::Ticker, tic %d, st %d, nplayers %d\n", gametic, state, n);
  if (n > 0)
    CONS_Printf("= playerstate = %d\n", players[0]->playerstate);

  // assign players to maps if needed
  // note! players vector must always contain only valid PI pointers! no NULLs!
  if (state == GS_LEVEL)
    {
      for (i=0; i<n; i++)
	{
	  switch (players[i]->playerstate)
	    {
	    case PST_WAITFORMAP:
	      /*
		if (!multiplayer && !cv_deathmatch.value)
		{
		// FIXME, not right. Map should call StartLevel
		// reload the level from scratch
		StartLevel(true, true);
		}
		else
	      */ 
	      {
		int m = 0;
		// just assign the player to a map
		maps[m]->AddPlayer(players[i]);
	      }
	      break;
	    case PST_REMOVE:
	      // the player is removed from the game
	      RemovePlayer(&players[i]);
	      n--; // FIXME not good, use an iterator or sth...
	      i--; // because the vector was shortened
	      break;
	    default:
	      break;
	    }
	}
    }


  int buf = gametic % BACKUPTICS;
  ticcmd_t *cmd;

  n = players.size();
  // read/write demo and check turbo cheat

  for (i=0 ; i<n ; i++)
    {
      // BP: i==0 for playback of demos 1.29 now new players is added with xcmd
      if (!dedicated)
        {
	  cmd = &players[i]->cmd;
	  
	  if (demoplayback)
	    ReadDemoTiccmd(cmd, i);
	  else
	    {
	      // FIXME here the netcode is bypassed until it's fixed. See also G_BuildTiccmd()!
	      //memcpy(cmd, &netcmds[buf][players[i]->number-1], sizeof(ticcmd_t));
	    }

	  if (demorecording)
	    WriteDemoTiccmd(cmd, i);
	
#define TURBOTHRESHOLD  0x32  
	  // check for turbo cheats FIXME.. move away from here!
	  if (cmd->forwardmove > TURBOTHRESHOLD
	      && !(gametic % (32*NEWTICRATERATIO)) && ((gametic / (32*NEWTICRATERATIO))&3) == i)
            {
	      static char turbomessage[80];
	      sprintf (turbomessage, "%s is turbo!", players[i]->name.c_str());
	      consoleplayer->message = turbomessage;
            }
        }
    }

  // do main actions
  switch (state)
    {
    case GS_LEVEL:
      //IO_Color(0,255,0,0);
      // FIXME !paused should be enough, menu should put the game on pause if possible
      // if (!paused && !(Menu::active && !netgame && !demoplayback))
      if (!paused)
	{
	  // FIXME just one map for now
	  maps[0]->Ticker(); // tic the maps
	  // FIXME Chasecam movement. Think over.
	  // Chasecam is an independent local actor, or ...?
	  // (local == not copied over network) It follows the displayplayer. Splitscreen?
	  if (camera.chase)
	    camera.MoveChaseCamera(displayplayer->pawn);
	}
      hud.Ticker();
      automap.Ticker();
      break;

    case GS_INTERMISSION:
      wi.Ticker();
      break;

    case GS_FINALE:
      F_Ticker();
      break;

    case GS_DEMOSCREEN:
      D_PageTicker();
      break;
      
    case GS_WAITINGPLAYERS:
    case GS_DEDICATEDSERVER:
    case GS_NULL:
    default:
      // do nothing
      break;
    }
  CONS_Printf("======== GI::Ticker done\n");
}

// starts a new local game
bool GameInfo::DeferredNewGame(skill_t sk, LevelNode *node, bool splitscreen)
{
  // if (n == NULL) do_something....
  // FIXME first we should check if we have all the WAD resources n requires
  // if (not enough resources) return false;
  firstlevel = node;

  Downgrade(VERSION);
  paused = false;
  nomonsters = false;
  skill = sk;

  if (demoplayback)
    COM_BufAddText ("stopdemo\n");

  // this leaves the actual game if needed
  SV_StartSinglePlayerServer();

  // delete old players
  ClearPlayers();

  // add local players
  consoleplayer = AddPlayer(-1, new PlayerInfo(localplayer));

  if (splitscreen)
    consoleplayer2 = AddPlayer(-1, new PlayerInfo(localplayer2));
  
  COM_BufAddText (va("splitscreen %d;deathmatch 0;fastmonsters 0;"
		     "respawnmonsters 0;timelimit 0;fraglimit 0\n",
		     splitscreen));

  COM_BufAddText("restartgame\n");
  return true;
}

// starts or restarts the game. firstlevel must be set.
bool GameInfo::StartGame()
{
  if (firstlevel == NULL)
    return false;

  NewLevel(skill, firstlevel, true);
  return true;
}
// was G_InitNew()
// Sets up and starts a new level inside a game.
//
// This is the map command interpretation (result of Command_Map_f())
//
// called at : map cmd execution, doloadgame, doplaydemo 

void GameInfo::NewLevel(skill_t sk, LevelNode *n, bool resetplayer)
{
  //char gamemapname[MAX_WADPATH];      // an external wad filename
  currentlevel = n;

  automap.Close();

  // delete old level
  int i;
  for (i = maps.size()-1; i>=0; i--)
    delete maps[i];
  maps.clear();

  if (players[0]->pawn) CONS_Printf("- NL 2: pawn->health = %d\n", players[0]->pawn->health);
  //added:27-02-98: disable selected features for compatibility with
  //                older demos, plus reset new features as default
  if (!Downgrade(demoversion))
    {
      CONS_Printf("Cannot Downgrade engine.\n");
      CL_Reset();
      StartIntro();
      return;
    }

  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  if (sk > sk_nightmare)
    sk = sk_nightmare;

  M_ClearRandom();

  if (server && sk == sk_nightmare)
    {
      CV_SetValue(&cv_respawnmonsters,1);
      CV_SetValue(&cv_fastmonsters,1);
    }

  if (fc.FindNumForName(n->mapname.c_str()) == -1)
    {
      // FIXME! this entire block
      //has the name got a dot (.) in it?
      if (!FIL_CheckExtension(n->mapname.c_str()))
	// append .wad to the name
	;

      // try to load the file
      if (true)
	CONS_Printf("\2Map '%s' not found\n"
		    "(use .wad extension for external maps)\n", n->mapname.c_str());
      Command_ExitGame_f();
      return;
    }

  skill = sk;

  // this should be CL_Reset or something...
  //playerdeadview = false;

  Map *m = new Map(n->mapname);
  m->level = n;
  maps.push_back(m); // just one map for now

  StartLevel(false, resetplayer);
}


//
// StartLevel : (re)starts the 'currentlevel' stored in 'maps' vector
// was G_DoLoadLevel()
// if re is true, do not reload the entire maps, just restart them.
void GameInfo::StartLevel(bool re, bool resetplayer)
{
  // FIXME make restart option work, do not FreeTags and Setup,
  // instead just Reset the maps
  int i;

  //levelstarttic = gametic;        // for time calculation

  if (wipestate == GS_LEVEL)
    wipestate = GS_WIPE;             // force a wipe

  state = GS_LEVEL;

  if (players[0]->pawn) CONS_Printf("- SL 1: pawn->health = %d\n", players[0]->pawn->health);
  int n = players.size();
  for (i=0 ; i<n ; i++)
    players[i]->Reset(resetplayer, cv_deathmatch.value ? false : true);

  //if (players[0]->pawn) CONS_Printf("- SL 2: pawn->health = %d\n", players[0]->pawn->health);
  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);

  if (players[0]->pawn) CONS_Printf("- SL 3: pawn->health = %d\n", players[0]->pawn->health);

  // set switch texture names/numbers (bad design..)
  P_InitSwitchList();

  // setup all maps in the level
  LevelNode *l = currentlevel;
  l->kills = l->items = l->secrets = 0;
  n = maps.size();
  for (i = 0; i<n; i++)
    {
      if (!(maps[i]->Setup(gametic)))
	{
	  // fail so reset game stuff
	  Command_ExitGame_f();
	  return;
	}
      l->kills += maps[i]->kills;
      l->items += maps[i]->items;
      l->secrets += maps[i]->secrets;
    }

  if (players[0]->pawn) CONS_Printf("- SL 4: pawn->health = %d\n", players[0]->pawn->health);
  //Fab:19-07-98:start cd music for this level (note: can be remapped)
  /*
    FIXME cd music
  if (game.mode==commercial)
    I_PlayCD (map, true);                // Doom2, 32 maps
  else
    I_PlayCD ((episode-1)*9+map, true);  // Doom1, 9maps per episode
  */

#ifdef FRAGGLESCRIPT
  T_PreprocessScripts();        // preprocess FraggleScript scripts (needs already added players)
  script_camera_on = false;
#endif

  //AM_LevelInit()
  //BOT_InitLevelBots ();

  displayplayer = consoleplayer;          // view the guy you are playing
  if (cv_splitscreen.value)
    displayplayer2 = consoleplayer2;
  else
    displayplayer2 = NULL;

  action = ga_nothing;

  if (players[0]->pawn) CONS_Printf("- SL 5: pawn->health = %d\n", players[0]->pawn->health);
#ifdef PARANOIA
  Z_CheckHeap(-2);
#endif

  // clear cmd building stuff
  memset(gamekeydown, 0, sizeof(gamekeydown));

  //joyxmove = joyymove = 0;
  //mousex = mousey = 0;
  if (players[0]->pawn) CONS_Printf("- SL 6: pawn->health = %d\n", players[0]->pawn->health);

  // clear hud messages remains (usually from game startup)
  CON_ClearHUD();
}


// was G_ExitLevel
//
// for now, exit can be 0 (normal exit) or 1 (secret exit)
void GameInfo::ExitLevel(int exit)
{
  if (state == GS_LEVEL) // FIXME! is this necessary?
    {
      action = ga_completed;
      currentlevel->exittype = exit;
    }
}

// Here's for the german edition.
/*
void G_SecretExitLevel (void)
{
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ((game.mode == commercial)
      && (W_CheckNumForName("map31")<0))
        secretexit = false;
    else
        secretexit = true;
    game.action = ga_completed;
}
*/


// was G_DoCompleted
//
// start intermission
void GameInfo::LevelCompleted()
{
  int i, n;

  // action is ga_completed
  action = ga_nothing;

  // save or forfeit pawns
  // player has a pawn if he is alive or dead, but not if he is in respawnqueue
  n = players.size();
  for (i=0 ; i<n ; i++)
    if (players[i]->playerstate == PST_LIVE) 
      {     
	CONS_Printf("- LC: pawn->health = %d\n", players[i]->pawn->health);
	//if (players[i]->pawn)
	players[i]->pawn->FinishLevel(); // take away cards and stuff, save the pawn
	CONS_Printf("- LC: pawn->health = %d\n", players[i]->pawn->health);
      }
    else
      players[i]->pawn = NULL; // let the pawn be destroyed with the map
  // TODO here we have a problem with hubs: if a player is dead when the hub is exited,
  // his corpse is never placed in bodyqueue and thus never flushed out...

  // compact the frags vectors, reassign player numbers? Maybe not.

  automap.Close();

  currentlevel->done = true;
  currentlevel->time = maps[0]->maptic / TICRATE;

  // dm nextmap wraparound?

  state = GS_INTERMISSION;

  //if (statcopy) memcpy(statcopy, currentlevel, sizeof(*currentlevel));

  wi.Start(currentlevel, firstlevel);
}


//
// was G_NextLevel (WorldDone)
//
// called when intermission ends
// init next level or go to the final scene
void GameInfo::EndIntermission()
{
  // action is ga_nothing

  // check need for finale
  if (cv_deathmatch.value == 0)
    {
      // check winning
      if (currentlevel->exit[currentlevel->exittype] == NULL)
	{
	  // disconnect from network
	  CL_Reset();
	  state = GS_FINALE;
	  F_StartFinale(currentlevel, true);
	  return;
	}

      // check "mid-game finale" (story)
      if (!currentlevel->finaletext.empty())
	{
	  state = GS_FINALE;
	  F_StartFinale(currentlevel, false);
	  return;
	}
    } else {
      // no finales in deathmatch
      // FIXME end game here, show final frags
      if (currentlevel->exit[currentlevel->exittype] == NULL)
	CL_Reset();
    }

  action = ga_worlddone;
}

void GameInfo::EndFinale()
{
  if (state == GS_FINALE) action = ga_worlddone;
}

// was G_DoWorldDone
// load next level

void GameInfo::WorldDone()
{
  action = ga_nothing;

  // maybe a state change here? Right now state is GS_INTERMISSION or GS_FINALE ??

  LevelNode *p = currentlevel->exit[currentlevel->exittype];

  /*
  if (demoversion < 129)
    {
      // FIXME! doesn't work! restarts the same level!
      StartLevel(true, false);
    }
    else */
  if (server && !demoplayback)
    // not in demo because demo have the mapcommand on it
    {
      bool reset = false;
      // resetplayer in deathmatch for more equality
      if (cv_deathmatch.value)
	reset = true;

      NewLevel(skill, p, reset);
      //COM_BufAddText (va("map \"%s\" -noresetplayers\n", currentlevel->mapname.c_str()));
      //COM_BufAddText (va("map \"%s\"\n", currentlevel->mapname.c_str())); 
    }
}