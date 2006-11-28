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
/// \brief Rendering main loop and setup, utility functions (BSP, geometry, trigonometry).

#include "doomdef.h"

#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "am_map.h"
#include "hud.h"
#include "p_camera.h"

#include "r_render.h"
#include "r_data.h"
#include "r_state.h"
#include "r_draw.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_things.h"
#include "i_video.h"

#include "w_wad.h"

#ifndef NO_OPENGL
#include "hardware/hwr_render.h"
#endif

#include "hardware/oglrenderer.hpp"

Rend R;
#ifndef NO_OPENGL
HWRend HWR;
#endif

angle_t G_ClipAimingPitch(angle_t pitch);


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW             2048

extern bool devparm;			//in d_main.cpp


int                     viewangleoffset = 0; // obsolete, for multiscreen setup...

// increment every time a check is made
int                     validcount = 1;



int                     centerx;
int                     centery;
int                     centerypsp;     //added:06-02-98:cf R_DrawPSprite

fixed_t                 centerxfrac;
fixed_t                 centeryfrac;
fixed_t                 projection;
//added:02-02-98:fixing the aspect ration stuff...
fixed_t                 projectiony;

// just for profiling purposes
int                     framecount;
int                     linecount;
int                     loopcount;

fixed_t                 viewcos;
fixed_t                 viewsin;

// 0 = high, 1 = low
int                     detailshift;

//
// precalculated math tables
//
angle_t                 clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int                     viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t                 xtoviewangle[MAXVIDWIDTH+1];


int         scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
int         scalelightfixed[MAXLIGHTSCALE];
int         zlight[LIGHTLEVELS][MAXLIGHTZ];
int         fixedcolormap;


// bumped light from gun blasts
int                     extralight;


//===========================================
//  client consvars
//===========================================

CV_PossibleValue_t viewsize_cons_t[]={{3,"MIN"},{12,"MAX"},{0,NULL}};
CV_PossibleValue_t detaillevel_cons_t[]={{0,"High"},{1,"Low"},{0,NULL}};

consvar_t cv_viewsize       = {"viewsize","10",CV_SAVE|CV_CALL,viewsize_cons_t,R_SetViewSize};      //3-12
consvar_t cv_detaillevel    = {"detaillevel","0",CV_SAVE|CV_CALL,detaillevel_cons_t,R_SetViewSize}; // UNUSED
consvar_t cv_scalestatusbar = {"scalestatusbar","0",CV_SAVE|CV_CALL,CV_YesNo,R_SetViewSize};
// consvar_t cv_fov = {"fov","2048", CV_CALL | CV_NOINIT, NULL, R_ExecuteSetViewSize};

void Translucency_OnChange();
void BloodTime_OnChange();
CV_PossibleValue_t bloodtime_cons_t[]={{1,"MIN"},{3600,"MAX"},{0,NULL}};

consvar_t cv_translucency  = {"translucency","1",CV_CALL|CV_SAVE,CV_OnOff, Translucency_OnChange};
// how much tics to last for the last (third) frame of blood (S_BLOODx)
consvar_t cv_splats    = {"splats","1",CV_SAVE,CV_OnOff};
consvar_t cv_bloodtime = {"bloodtime","20",CV_CALL|CV_SAVE,bloodtime_cons_t,BloodTime_OnChange};
consvar_t cv_psprites  = {"playersprites","1",0,CV_OnOff};

CV_PossibleValue_t viewheight_cons_t[]={{16,"MIN"},{56,"MAX"},{0,NULL}};
consvar_t cv_viewheight = {"viewheight", "41",0,viewheight_cons_t,NULL};


//===========================================

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int R_PointOnSide(fixed_t x, fixed_t y, node_t *node)
{
  if (!node->dx)
    {
      if (x <= node->x)
	return node->dy > 0;

      return node->dy < 0;
    }
  if (!node->dy)
    {
      if (y <= node->y)
	return node->dx < 0;

      return node->dx > 0;
    }

  fixed_t dx = (x - node->x);
  fixed_t dy = (y - node->y);

  // Try to quickly decide by looking at sign bits.
  if ((node->dy.value() ^ node->dx.value() ^ dx.value() ^ dy.value()) & 0x80000000)
    {
      if ( (node->dy.value() ^ dx.value()) & 0x80000000 )
        {
	  // (left is negative)
	  return 1;
        }
      return 0;
    }

  fixed_t left = (node->dy >> fixed_t::FBITS) * dx;
  fixed_t right = dy * (node->dx >> fixed_t::FBITS);

  if (right < left)
    {
      // front side
      return 0;
    }
  // back side
  return 1;
}


int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
  fixed_t     lx;
  fixed_t     ly;
  fixed_t     ldx;
  fixed_t     ldy;
  fixed_t     dx;
  fixed_t     dy;
  fixed_t     left;
  fixed_t     right;

  lx = line->v1->x;
  ly = line->v1->y;

  ldx = line->v2->x - lx;
  ldy = line->v2->y - ly;

  if (!ldx)
    {
      if (x <= lx)
	return ldy > 0;

      return ldy < 0;
    }
  if (!ldy)
    {
      if (y <= ly)
	return ldx < 0;

      return ldx > 0;
    }

  dx = (x - lx);
  dy = (y - ly);

  // Try to quickly decide by looking at sign bits.
  if ( (ldy.value() ^ ldx.value() ^ dx.value() ^ dy.value())&0x80000000 )
    {
      if  ( (ldy.value() ^ dx.value()) & 0x80000000 )
        {
	  // (left is negative)
	  return 1;
        }
      return 0;
    }

  left = (ldy>>fixed_t::FBITS) * dx;
  right = dy * (ldx>>fixed_t::FBITS);

  if (right < left)
    {
      // front side
      return 0;
    }
  // back side
  return 1;
}



angle_t Rend::R_PointToAngle(fixed_t x, fixed_t y)
{
  return R_PointToAngle2(viewx, viewy, x, y);
}


fixed_t Rend::R_PointToDist(fixed_t x, fixed_t y)
{
  return R_PointToDist2(viewx, viewy, x, y);
}



//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
//added:02-02-98:note: THIS IS USED ONLY FOR WALLS!
fixed_t Rend::R_ScaleFromGlobalAngle(angle_t visangle)
{
  // UNUSED
#if 0
  //added:02-02-98:note: I've tried this and it displays weird...
  fixed_t             scale;
  fixed_t             dist;
  fixed_t             z;
  fixed_t             sinv;
  fixed_t             cosv;

  sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
  dist = rw_distance / sinv;
  cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
  z = abs(FixedMul(dist, cosv));
  scale = projection / z;
  return scale;

#else
  fixed_t             scale;

  int anglea = ANG90 + (visangle-viewangle);
  int angleb = ANG90 + (visangle-rw_normalangle);

    // both sines are allways positive
  fixed_t sinea = finesine[anglea>>ANGLETOFINESHIFT];
  fixed_t sineb = finesine[angleb>>ANGLETOFINESHIFT];
  //added:02-02-98:now uses projectiony instead of projection for
  //               correct aspect ratio!
  fixed_t num = (projectiony * sineb)<<detailshift;
  fixed_t den = rw_distance * sinea;

  if (den > num>>16)
    {
      scale = num/den;

      if (scale > 64)
	scale = 64;
      else if (scale.value() < 256)
	scale.setvalue(256);
    }
  else
    scale = 64;

  return scale;
#endif
}




//
// R_InitTextureMapping
//
void R_InitTextureMapping()
{
  int  i, t;

  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  //
  // Calc focallength
  //  so FIELDOFVIEW angles covers SCREENWIDTH.
  fixed_t focallength = centerxfrac / finetangent[FINEANGLES/4+/*cv_fov.value*/ FIELDOFVIEW/2];

  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      if (finetangent[i] > 2)
	t = -1;
      else if (finetangent[i] < -2)
	t = viewwidth+1;
      else
        {
	  fixed_t temp = 1 - fixed_epsilon;
	  temp -= finetangent[i] * focallength;
	  t = (centerxfrac + temp).floor();

	  if (t < -1)
	    t = -1;
	  else if (t>viewwidth+1)
	    t = viewwidth+1;
        }
      viewangletox[i] = t;
    }

  // Scan viewangletox[] to generate xtoviewangle[]:
  //  xtoviewangle will give the smallest view angle
  //  that maps to x.
  for (int x = 0; x <= viewwidth; x++)
    {
      i = 0;
      while (viewangletox[i]>x)
	i++;
      xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

  // Take out the fencepost cases from viewangletox.
  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      t = (centerx - (finetangent[i] * focallength)).value(); // FIXME not used!?

      if (viewangletox[i] == -1)
	viewangletox[i] = 0;
      else if (viewangletox[i] == viewwidth+1)
	viewangletox[i]  = viewwidth;
    }

  clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP         2

void R_InitLightTables()
{
  // Calculate the light levels to use
  //  for each level / distance combination.
  for (int i=0 ; i< LIGHTLEVELS ; i++)
    {
      int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (int j=0 ; j<MAXLIGHTZ ; j++)
        {
	  //added:02-02-98:use BASEVIDWIDTH, vid.width is not set already,
	  // and it seems it needs to be calculated only once.
	  int scale = (fixed_t(BASEVIDWIDTH/2) / fixed_t(j+1)).floor();
	  int level = startmap - scale/DISTMAP;

	  if (level < 0)
	    level = 0;

	  if (level >= NUMCOLORMAPS)
	    level = NUMCOLORMAPS-1;

	  zlight[i][j] = level*256;
        }
    }
}


//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
bool         setsizeneeded;

void R_SetViewSize()
{
  setsizeneeded = true;
}


//
// R_ExecuteSetViewSize
//
// now uses screen variables cv_viewsize, cv_detaillevel
//
void R_ExecuteSetViewSize()
{
  int i, j;

  // no reduced view in splitscreen mode
  if (cv_splitscreen.value && cv_viewsize.value < 11)
    cv_viewsize.Set(11);

#ifndef NO_OPENGL
  if ((rendermode != render_soft) && (cv_viewsize.value < 6))
    cv_viewsize.Set(6);
#endif

  setsizeneeded = false;

  hud.ST_Recalc();

  //added 01-01-98: full screen view, without statusbar
  if (cv_viewsize.value > 10)
    {
      scaledviewwidth = vid.width;
      viewheight = vid.height;
    }
  else
    {
      //added 01-01-98: always a multiple of eight
      scaledviewwidth = (cv_viewsize.value * vid.width/10) & ~7;
      //added:05-02-98: make viewheight multiple of 2 because sometimes
      //                a line is not refreshed by R_DrawViewBorder()
      viewheight = (cv_viewsize.value*(vid.height-hud.stbarheight)/10) & ~1;
    }

  // added 16-6-98:splitscreen
  if (cv_splitscreen.value)
    viewheight >>= 1;

  int setdetail = cv_detaillevel.value;
  // clamp detail level (actually ignore it, keep it for later who knows)
  if (setdetail)
    {
      setdetail = 0;
      CONS_Printf("lower detail mode n.a.\n");
      cv_detaillevel.Set(setdetail);
    }

  detailshift = setdetail;
  viewwidth = scaledviewwidth>>detailshift;

  centery = viewheight/2;
  centerx = viewwidth/2;
  centerxfrac = centerx;
  centeryfrac = centery;

  //added:01-02-98:aspect ratio is now correct, added an 'projectiony'
  //      since the scale is not always the same between horiz. & vert.
  projection  = centerxfrac;
  projectiony = ((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width;

  //
  // no more low detail mode, it used to setup the right drawer routines
  // for either detail mode here
  //
  // if (!detailshift) ... else ...

  R_InitViewBuffer(scaledviewwidth, viewheight);

  R_InitTextureMapping();

  // psprite scales
  centerypsp = viewheight/2;  //added:06-02-98:psprite pos for freelook

  pspritescale  = fixed_t(viewwidth) / BASEVIDWIDTH;
  pspriteiscale = fixed_t(BASEVIDWIDTH) / viewwidth;   // x axis scale
  //added:02-02-98:now aspect ratio correct for psprites
  pspriteyscale = fixed_t((vid.height*viewwidth)/vid.width)/BASEVIDHEIGHT;

  // thing clipping
  for (i=0 ; i<viewwidth ; i++)
    screenheightarray[i] = viewheight;

  // planes
  if (rendermode == render_soft)
    {
      //added:02-02-98:now correct aspect ratio!
      int aspectx = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width);

      // this is only used for planes rendering in software mode
      j = viewheight*4;
      for (i=0 ; i<j ; i++)
	{
	  //added:10-02-98:(i-centery) became (i-centery*2) and centery*2=viewheight
	  fixed_t dy = abs(fixed_t(i - viewheight*2) + 0.5f);
	  yslopetab[i] = aspectx / dy;
	}
    }

  for (i=0 ; i<viewwidth ; i++)
    {
      fixed_t cosadj = abs(Cos(xtoviewangle[i]));
      distscale[i] = 1 / cosadj;
    }

  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0 ; i< LIGHTLEVELS ; i++)
    {
      int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
	  int level = startmap - j*vid.width/(viewwidth<<detailshift)/DISTMAP;

	  if (level < 0)
	    level = 0;

	  if (level >= NUMCOLORMAPS)
	    level = NUMCOLORMAPS-1;

	  scalelight[i][j] = level*256;
        }
    }

  //faB: continue to do the software setviewsize as long as we use
  //     the reference software view
#ifndef NO_OPENGL
  if (rendermode!=render_soft)
    HWR.SetViewSize(cv_viewsize.value);
#endif

  automap.Resize();
}


//
// R_Init
//

static void TestAnims()
{
  // This function can be used to test the integrity of the mobj states (sort of)

  int i, j, k, l, spr;
  mobjinfo_t *info;
  state_t *s, *n;

  state_t *mobjinfo_t::*seqptr[9] =
  {
    &mobjinfo_t::spawnstate,
    &mobjinfo_t::seestate,
    &mobjinfo_t::painstate,
    &mobjinfo_t::meleestate,
    &mobjinfo_t::missilestate,
    &mobjinfo_t::deathstate,
    &mobjinfo_t::xdeathstate,
    &mobjinfo_t::crashstate,
    &mobjinfo_t::raisestate
  };
  const char *snames[9] =
  {
    "spawn  ",
    "see    ",
    "pain   ",
    "melee  ",
    "missile",
    "death  ",
    "xdeath ",
    "crash  ",
    "raise  "
  };

  state_t *seq[9];

  for (i=0; i<NUMMOBJTYPES; i++)
    {
      info = &mobjinfo[i];

      s = info->spawnstate;
      spr = s->sprite;
      printf("\n%d: %s\n", i, sprnames[spr]);

      for (j = 0; j<9; j++)
	seq[j] = info->*seqptr[j];

      for (j = 0; j<9; j++)
	{
	  s = n = seq[j];
	  printf(" %s: ", snames[j]);
	  if (!n)
	    {
	      printf("(none)\n");
	      continue;
	    }

	  for (k = 0; k < 40; k++)
	    {
	      if (n == &states[S_NULL])
		{
		  printf("S_NULL, %d\n", k);
		  break;
		}

	      if (n->sprite != spr)
		{
		  printf("! name: %s: ", sprnames[n->sprite]);
		  spr = n->sprite;
		}

	      if (n->tics < 0)
		{
		  printf("hold, %d\n", k+1);
		  break;
		}

	      n = n->nextstate;
	      if (n == s)
		{
		  printf("loop, %d\n", k+1);
		  break;
		}
	      else for (l=0; l<9; l++)
		if (n == seq[l] && n != &states[S_NULL])
		  break;

	      if (l < 9)
		{
		  printf("6-loop to %s, %d+\n", snames[l], k+1);
		  break;
		}
	    }
	  if (k == 40)
	    printf("l >= 40 !!!\n");
	}
    }

  I_Error("\n ... done.\n");
}




/// Initializes the essential parts of the renderer
/// (even a dedicated server needs these)
void R_ServerInit()
{
  // server needs to know the texture names and dimensions
  CONS_Printf("InitTextures...\n");
  tc.Clear();
  tc.SetDefaultItem("DEF_TEX");
  tc.ReadTextures();

  //tc.Inventory();

  // set the default items for sprite and model caches
  CONS_Printf("InitSprites...\n");
  R_InitSprites(sprnames);
}


int P_Read_ANIMATED(int lump);
int P_Read_ANIMDEFS(int lump);

/// Initializes the client renderer.
/// The server part has already been initialized in R_ServerInit.
void R_Init()
{
  //TestAnims();

  // Read texture animations, insert them into the cache, replacing the originals.
  if (P_Read_ANIMDEFS(fc.FindNumForName("ANIMDEFS")) < 0)
    P_Read_ANIMATED(fc.FindNumForName("ANIMATED"));

  // prepare the window border textures
  R_InitViewBorder();

  // setsizeneeded is set true, the viewport params will be recalculated before next rendering.
  R_SetViewSize();

  // load lightlevel colormaps and Boom extra colormaps
  if (devparm)
    CONS_Printf("InitColormaps...\n");
  R_InitColormaps();

  // initialize sw renderer lightlevel tables (colormaps...)
  if (devparm)
    CONS_Printf("InitLightTables...\n");
  R_InitLightTables();

  // load playercolor translation colormaps
  if (devparm)
    CONS_Printf("InitTranslationTables...\n");
  R_InitTranslationTables();

  // load or create translucency tables
  if (devparm)
    CONS_Printf("InitTranslucencyTables...\n");
  R_InitTranslucencyTables();

  R_InitDrawNodes();

  framecount = 0;
}


//
// was R_PointInSubsector
//
subsector_t *Map::R_PointInSubsector(fixed_t x, fixed_t y)
{
  // single subsector is a special case
  if (!numnodes)
    return subsectors;

  int nodenum = numnodes-1;

  while (! (nodenum & NF_SUBSECTOR) )
    {
      node_t *node = &nodes[nodenum];
      int side = R_PointOnSide (x, y, node);
      nodenum = node->children[side];
    }

  return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// was R_IsPointInSubsector, same of above but return 0 if not in subsector
//
subsector_t* Map::R_IsPointInSubsector(fixed_t x, fixed_t y)
{
  // single subsector is a special case
  if (!numnodes)
    return subsectors;

  int nodenum = numnodes-1;

  while (! (nodenum & NF_SUBSECTOR) )
    {
      node_t *node = &nodes[nodenum];
      int side = R_PointOnSide (x, y, node);
      nodenum = node->children[side];
    }

  subsector_t *ret = &subsectors[nodenum & ~NF_SUBSECTOR];
  for (unsigned i=0; i<ret->num_segs; i++)
    {
      if (R_PointOnSegSide(x,y,&segs[ret->first_seg + i]))
	return 0;
    }

  return ret;
}


//
// R_SetupFrame
//
bool drawPsprites; // FIXME HACK


void Rend::R_SetupFrame(PlayerInfo *player)
{
  extralight = player->pawn->extralight;

  viewplayer = player->pawn; // for colormap effects due to IR visor etc.
  viewactor  = player->pov; // the point of view for this player (usually same as pawn, but may be a camera too)

  viewx = viewactor->pos.x;
  viewy = viewactor->pos.y;

  drawPsprites = (viewactor == viewplayer);
  if (drawPsprites)
    viewz = player->viewz; // enable bobbing
  else
    viewz = viewactor->pos.z;

  int fixedcolormap_setup = player->pawn->fixedcolormap;
  //fixedcolormap_setup = script_camera->fixedcolormap;

  viewangle = viewactor->yaw + viewangleoffset;
  aimingangle = viewactor->pitch;

  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

  if (fixedcolormap_setup)
    {
      fixedcolormap = fixedcolormap_setup*256*sizeof(lighttable_t);

      walllights = scalelightfixed;

      for (int i=0 ; i<MAXLIGHTSCALE ; i++)
        scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

  //added:06-02-98:recalc necessary stuff for mouseaiming
  //               slopes are already calculated for the full
  //               possible view (which is 4*viewheight).

  int dy = 0;

  if ( rendermode == render_soft )
    {
      // clip it in the case we are looking a hardware 90� full aiming
      // (lmps, nework and use F12...)
      aimingangle = G_ClipAimingPitch(aimingangle);

      // WARNING : a should be unsigned but to add with 2048, it isn't !
#define AIMINGTODY(a) (finetangent[(2048+(int(a) >> ANGLETOFINESHIFT)) & FINEMASK] * 160).floor()

      if(!cv_splitscreen.value)
        dy = AIMINGTODY(aimingangle)* viewheight/BASEVIDHEIGHT ;
      else
        dy = AIMINGTODY(aimingangle)* viewheight*2/BASEVIDHEIGHT ;

      yslope = &yslopetab[(3*viewheight/2) - dy];
    }
  centery = (viewheight/2) + dy;
  centeryfrac = centery;

  framecount++;
  validcount++;
}



// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void Rend::R_RenderPlayerView(int viewport, PlayerInfo *player)
{
  SetMap(player->mp);
  R_SetupFrame(player); // some of this is needed for OpenGL too!

  // OpenGL
  if (rendermode == render_opengl)
    {
      oglrenderer->Render3DView(player);
      // Draw weapon sprites. 
      R_DrawPlayerSprites();
      return;
    }



  if (viewport == 0) // support just two viewports for now
    {
      ylookup = ylookup1;
    }
  else
    {
      //faB: Boris hack :P !!
      viewwindowy = vid.height/2;
      ylookup = ylookup2;
    }


  // Clear buffers.
  R_ClearClipSegs();
  R_ClearDrawSegs();
  R_ClearPlanes();
  //R_ClearPortals();
  R_ClearSprites();

#ifdef FLOORSPLATS
  R_ClearVisibleFloorSplats();
#endif

  // check for new console commands.
  //NetUpdate ();

  // The head node is the last node output.

  //profile stuff ---------------------------------------------------------
#ifdef TIMING
  mytotal=0;
  ProfZeroTimer();
#endif

  R_RenderBSPNode(numnodes-1);

#ifdef TIMING
  RDMSR(0x10,&mycount);
  mytotal += mycount;   //64bit add
  CONS_Printf("RenderBSPNode: 0x%d %d\n", *((int*)&mytotal+1), (int)mytotal );
#endif
  //profile stuff ---------------------------------------------------------

  // Check for new console commands.
  //NetUpdate ();

  //R_DrawPortals();
  R_DrawPlanes();

  // Check for new console commands.
  //NetUpdate ();

#ifdef FLOORSPLATS
  //faB(21jan): testing
  R_DrawVisibleFloorSplats();
#endif

  // draw mid texture and sprite
  // SoM: And now 3D floors/sides!
  R_DrawMasked();

  // draw the psprites on top of everything
  //  but does not draw on side views
  if (!viewangleoffset && cv_psprites.value && drawPsprites)
    R_DrawPlayerSprites();

  // Check for new console commands.
  //NetUpdate ();
  player->pawn->flags &= ~MF_NOSECTOR; // don't show self (uninit) clientprediction code

  if (viewport != 0) // HACK support just two viewports for now
    {
      viewwindowy = 0;
      ylookup = ylookup1;
    }
}
