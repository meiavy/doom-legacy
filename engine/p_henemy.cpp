// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// Revision 1.1  2002/11/16 14:17:57  hurdler
// Initial revision
//
// Revision 1.13  2002/09/20 22:41:30  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.12  2002/08/23 09:53:41  vberghol
// fixed Actor:: target/owner/tracer
//
// Revision 1.11  2002/08/17 21:21:47  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.10  2002/08/11 17:16:48  vberghol
// ...
//
// Revision 1.9  2002/08/08 12:01:26  vberghol
// pian engine on valmis!
//
// Revision 1.8  2002/08/06 13:14:22  vberghol
// ...
//
// Revision 1.7  2002/08/02 20:14:49  vberghol
// p_enemy.cpp done!
//
// Revision 1.6  2002/07/26 19:23:04  vberghol
// a little something
//
// Revision 1.5  2002/07/23 19:21:41  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.4  2002/07/12 19:21:38  vberghol
// hop
//
// Revision 1.3  2002/07/01 21:00:17  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:13  vberghol
// Version 133 Experimental!
//
// Revision 1.6  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.5  2001/05/27 13:42:47  bpereira
// no message
//
// Revision 1.4  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.3  2001/02/24 13:35:20  bpereira
// no message
//
//
//
// DESCRIPTION:
//   Like p_enemy.cpp but for Heretic
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"
#include "p_enemy.h"
#include "p_maputl.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"
#include "m_fixed.h"
#include "tables.h"

// here was P_InitMonsters
// here was P_AddBossSpot


//=========================================
//   ACTION ROUTINES
//=========================================

//----------------------------------------------------------------------------
//
// PROC A_DripBlood
//
//----------------------------------------------------------------------------

void A_DripBlood(Actor *actor)
{
  Actor *mo;
  int r,s;

  // evaluation order isn't define in C
  r = P_SignedRandom();
  s = P_SignedRandom();
  mo = actor->mp->SpawnActor(actor->x+(r<<11),
		   actor->y+(s<<11), actor->z, MT_BLOOD);
  mo->px = P_SignedRandom()<<10;
  mo->py = P_SignedRandom()<<10;
  mo->flags2 |= MF2_LOGRAV;
}

//----------------------------------------------------------------------------
//
// PROC A_KnightAttack
//
//----------------------------------------------------------------------------

void A_KnightAttack(Actor *actor)
{
  if (!actor->target)
    {
      return;
    }
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(3));
      S_StartSound(actor, sfx_kgtat2);
      return;
    }
  // Throw axe
  S_StartSound(actor, actor->info->attacksound);
  if(actor->type == MT_KNIGHTGHOST || P_Random() < 40)
    { // Red axe
      actor->SpawnMissile(actor->target, MT_REDAXE);
      return;
    }
  // Green axe
  actor->SpawnMissile(actor->target, MT_KNIGHTAXE);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpExplode
//
//----------------------------------------------------------------------------

void A_ImpExplode(Actor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_IMPCHUNK1);
  mo->px = P_SignedRandom()<<10;
  mo->py = P_SignedRandom()<<10;
  mo->pz = 9*FRACUNIT;
  mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_IMPCHUNK2);
  mo->px = P_SignedRandom()<<10;
  mo->py = P_SignedRandom()<<10;
  mo->pz = 9*FRACUNIT;
  if (actor->special1 == 666)
    { // Extreme death crash
      actor->SetState(S_IMP_XCRASH1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BeastPuff
//
//----------------------------------------------------------------------------

void A_BeastPuff(Actor *actor)
{
  if(P_Random() > 64)
    {
      int r,s,t;
      r = P_SignedRandom();
      s = P_SignedRandom();
      t = P_SignedRandom();
        
      actor->mp->SpawnActor(actor->x+(r<<10),
		  actor->y+(s<<10),
		  actor->z+(t<<10), MT_PUFFY);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMeAttack
//
//----------------------------------------------------------------------------

void A_ImpMeAttack(Actor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, 5+(P_Random()&7));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack
//
//----------------------------------------------------------------------------

void A_ImpMsAttack(Actor *actor)
{
  Actor *dest;
  angle_t an;
  int dist;

  if(!actor->target || P_Random() > 64)
    {
      actor->SetState(actor->info->seestate);
      return;
    }
  dest = actor->target;
  actor->flags |= MF_SKULLFLY;
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  an = actor->angle >> ANGLETOFINESHIFT;
  actor->px = FixedMul(12*FRACUNIT, finecosine[an]);
  actor->py = FixedMul(12*FRACUNIT, finesine[an]);
  dist = P_AproxDistance(dest->x-actor->x, dest->y-actor->y);
  dist = dist/(12*FRACUNIT);
  if(dist < 1)
    {
      dist = 1;
    }
  actor->pz = (dest->z+(dest->height>>1)-actor->z)/dist;
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack2
//
// Fireball attack of the imp leader.
//
//----------------------------------------------------------------------------

void A_ImpMsAttack2(Actor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, 5+(P_Random()&7));
      return;
    }
  actor->SpawnMissile(actor->target, MT_IMPBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpDeath
//
//----------------------------------------------------------------------------

void A_ImpDeath(Actor *actor)
{
  actor->flags &= ~MF_SOLID;
  actor->flags2 |= MF2_FOOTCLIP;

  actor->flags  |= MF_CORPSE|MF_DROPOFF;
  actor->height >>= 2;
  actor->radius -= (actor->radius>>4);      //for solid corpses

  if(actor->z <= actor->floorz)
    {
      actor->SetState(S_IMP_CRASH1);
      actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath1
//
//----------------------------------------------------------------------------

void A_ImpXDeath1(Actor *actor)
{
  actor->flags &= ~MF_SOLID;
  actor->flags |= MF_NOGRAVITY;
  actor->flags  |= MF_CORPSE|MF_DROPOFF;
  actor->height >>= 2;
  actor->radius -= (actor->radius>>4);      //for solid corpses

  actor->flags2 |= MF2_FOOTCLIP;
  actor->special1 = 666; // Flag the crash routine
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath2
//
//----------------------------------------------------------------------------

void A_ImpXDeath2(Actor *actor)
{
  actor->flags &= ~MF_NOGRAVITY;
  if(actor->z <= actor->floorz)
    {
      actor->SetState(S_IMP_CRASH1);
      actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// was P_UpdateChicken
//
// Returns true if the chicken morphs.
//
//----------------------------------------------------------------------------

#define TELEFOGHEIGHT (32*FRACUNIT)
bool Actor::UpdateMorph(int tics)
{
  special1 -= tics;
  if (special1 > 0)
    return false;  

  // undo the morph
  mobjtype_t moType = mobjtype_t(special2);

  Actor *mo = mp->SpawnActor(x, y, z, moType);
  
  if (mo->TestLocation() == false)
    { // Didn't fit
      mo->Remove();
      special1 = 5*TICRATE; // Next try in 5 seconds
      return false;
    }
  mo->Remove(); // just used to see if it fits
  // FIXME! what is a freetargmobj?
  // also repair everything else here;)
  //mo->SetState(S_FREETARGMOBJ); //maybe?
  type = moType;
  info = &mobjinfo[type];

  Actor *fog = mp->SpawnActor(x, y, z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);

  return true;
  /*

  fixed_t x, y, z;
  Actor oldChicken;

  this->special1 -= tics;
  if (this->special1 > 0)
    return false;

  moType = mobjtype_t(this->special2);
  x = this->x;
  y = this->y;
  z = this->z;
  oldChicken = *this;
  this->SetState(S_FREETARGMOBJ);
  mo = P_SpawnMobj(x, y, z, moType);
  if (mo->TestLocation() == false)
    { // Didn't fit
      mo->Remove();
      mo = P_SpawnMobj(x, y, z, MT_CHICKEN);
      mo->angle = oldChicken.angle;
      mo->flags = oldChicken.flags;
      mo->health = oldChicken.health;
      mo->target = oldChicken.target;
      mo->special1 = 5*TICRATE; // Next try in 5 seconds
      mo->special2 = moType;
      return false;
    }
  mo->angle = oldChicken.angle;
  mo->target = oldChicken.target;
  fog = P_SpawnMobj(x, y, z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);
  */
}

//----------------------------------------------------------------------------
//
// PROC A_ChicAttack
//
//----------------------------------------------------------------------------

void A_ChicAttack(Actor *actor)
{
  if (actor->UpdateMorph(18))
    return;

  if(!actor->target)
      return;

  if(actor->CheckMeleeRange())
    actor->target->Damage(actor, actor, 1+(P_Random()&1));
}

//----------------------------------------------------------------------------
//
// PROC A_ChicLook
//
//----------------------------------------------------------------------------

void A_ChicLook(Actor *actor)
{
  if (actor->UpdateMorph(10))
    {
      return;
    }
  A_Look(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicChase
//
//----------------------------------------------------------------------------

void A_ChicChase(Actor *actor)
{
  if (actor->UpdateMorph(3))
    {
      return;
    }
  A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicPain
//
//----------------------------------------------------------------------------

void A_ChicPain(Actor *actor)
{
  if(actor->UpdateMorph(10))
    {
      return;
    }
  S_StartSound(actor, actor->info->painsound);
}

//----------------------------------------------------------------------------
//
// PROC A_Feathers
//
//----------------------------------------------------------------------------

void A_Feathers(Actor *actor)
{
  int i;
  int count;
  Actor *mo;

  if(actor->health > 0)
    { // Pain
      count = P_Random() < 32 ? 2 : 1;
    }
  else
    { // Death
      count = 5+(P_Random()&3);
    }
  for(i = 0; i < count; i++)
    {
      mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z+20*FRACUNIT,
		       MT_FEATHER);
      mo->target = actor;
      mo->px = P_SignedRandom()<<8;
      mo->py = P_SignedRandom()<<8;
      mo->pz = FRACUNIT+(P_Random()<<9);
      mo->SetState(statenum_t(S_FEATHER1+(P_Random()&7)));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack
//
//----------------------------------------------------------------------------

void A_MummyAttack(Actor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
      S_StartSound(actor, sfx_mumat2);
      return;
    }
  S_StartSound(actor, sfx_mumat1);
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack2
//
// Mummy leader missile attack.
//
//----------------------------------------------------------------------------

void A_MummyAttack2(Actor *actor)
{
  if (!actor->target)
    return;

  //S_StartSound(actor, actor->info->attacksound);
  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
      return;
    }
  Actor *mo = actor->SpawnMissile(actor->target, MT_MUMMYFX1);
  //mo = actor->SpawnMissile(actor->target, MT_EGGFX);
  if (mo != NULL)
    mo->target = actor->target;
}

//----------------------------------------------------------------------------
//
// PROC A_MummyFX1Seek
//
//----------------------------------------------------------------------------

void A_MummyFX1Seek(Actor *actor)
{
  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*20);
}

//----------------------------------------------------------------------------
//
// PROC A_MummySoul
//
//----------------------------------------------------------------------------

void A_MummySoul(Actor *mummy)
{
  Actor *mo;

  mo = mummy->mp->SpawnActor(mummy->x, mummy->y, mummy->z+10*FRACUNIT, MT_MUMMYSOUL);
  mo->pz = FRACUNIT;
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Pain
//
//----------------------------------------------------------------------------

void A_Sor1Pain(Actor *actor)
{
  actor->special1 = 20; // Number of steps to walk fast
  A_Pain(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Chase
//
//----------------------------------------------------------------------------

void A_Sor1Chase(Actor *actor)
{
  if(actor->special1)
    {
      actor->special1--;
      actor->tics -= 3;
    }
  A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr1Attack
//
// Sorcerer demon attack.
//
//----------------------------------------------------------------------------

void A_Srcr1Attack(Actor *actor)
{
  Actor *mo;
  fixed_t pz;
  angle_t angle;

  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(8));
      return;
    }
  if(actor->health > (actor->info->spawnhealth/3)*2)
    { // Spit one fireball
      actor->SpawnMissile(actor->target, MT_SRCRFX1);
    }
  else
    { // Spit three fireballs
      mo = actor->SpawnMissile(actor->target, MT_SRCRFX1);
      if(mo)
	{
	  pz = mo->pz;
	  angle = mo->angle;
	  actor->SpawnMissileAngle(MT_SRCRFX1, angle-ANGLE_1*3, pz);
	  actor->SpawnMissileAngle(MT_SRCRFX1, angle+ANGLE_1*3, pz);
	}
      if(actor->health < actor->info->spawnhealth/3)
	{ // Maybe attack again
	  if(actor->special1)
	    { // Just attacked, so don't attack again
	      actor->special1 = 0;
	    }
	  else
	    { // Set state to attack again
	      actor->special1 = 1;
	      actor->SetState(S_SRCR1_ATK4);
	    }
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SorcererRise
//
//----------------------------------------------------------------------------

void A_SorcererRise(Actor *actor)
{
  Actor *mo;

  actor->flags &= ~MF_SOLID;
  mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_SORCERER2);
  mo->SetState(S_SOR2_RISE1);
  mo->angle = actor->angle;
  mo->target = actor->target;
}

//----------------------------------------------------------------------------
//
// was P_DSparilTeleport
//
//----------------------------------------------------------------------------

void Actor::DSparilTeleport()
{
  int i, n;
  fixed_t nx, ny;
  fixed_t prevX, prevY, prevZ;
  Actor *mo;

  n = mp->BossSpots.size(); 
  if (n == 0)
    { // No spots
      return;
    }
  i = P_Random();
  do {
    i++;
    nx = mp->BossSpots[i%n]->x << FRACBITS;;
    ny = mp->BossSpots[i%n]->y << FRACBITS;;
  } while(P_AproxDistance(x-nx, y-ny) < 128*FRACUNIT);
  prevX = x;
  prevY = y;
  prevZ = z;
  if (TeleportMove(nx, ny))
    {
      mo = mp->SpawnActor(prevX, prevY, prevZ, MT_SOR2TELEFADE);
      S_StartSound(mo, sfx_telept);
      SetState(S_SOR2_TELE1);
      S_StartSound(this, sfx_telept);
      z = floorz;
      angle = ANG45 * (mp->BossSpots[i%n]->angle/45);
      px = py = pz = 0;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Decide
//
//----------------------------------------------------------------------------

void A_Srcr2Decide(Actor *actor)
{
  static int chance[] =
  {
    192, 120, 120, 120, 64, 64, 32, 16, 0
  };

  if (actor->mp->BossSpots.size() == 0)
    { // No spots
      return;
    }
  if(P_Random() < chance[actor->health/(actor->info->spawnhealth/8)])
    {
      actor->DSparilTeleport();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Attack
//
//----------------------------------------------------------------------------

void A_Srcr2Attack(Actor *actor)
{
  int chance;

  if(!actor->target)
    {
      return;
    }
  S_StartAmbSound(actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(20));
      return;
    }
  chance = actor->health < actor->info->spawnhealth/2 ? 96 : 48;
  if(P_Random() < chance)
    { // Wizard spawners
      actor->SpawnMissileAngle(MT_SOR2FX2,
			  actor->angle-ANG45, FRACUNIT/2);
      actor->SpawnMissileAngle(MT_SOR2FX2,
			  actor->angle+ANG45, FRACUNIT/2);
    }
  else
    { // Blue bolt
      actor->SpawnMissile(actor->target, MT_SOR2FX1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BlueSpark
//
//----------------------------------------------------------------------------

void A_BlueSpark(Actor *actor)
{
  int i;
  Actor *mo;
    
  for(i = 0; i < 2; i++)
    {
      mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_SOR2FXSPARK);
      mo->px = P_SignedRandom()<<9;
      mo->py = P_SignedRandom()<<9;
      mo->pz = FRACUNIT+(P_Random()<<8);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GenWizard
//
//----------------------------------------------------------------------------

void A_GenWizard(Actor *actor)
{
  Actor *mo;
  Actor *fog;

  mo = actor->mp->SpawnActor(actor->x, actor->y,
		   actor->z-mobjinfo[MT_WIZARD].height/2, MT_WIZARD);
  if(mo->TestLocation() == false)
    { // Didn't fit
      mo->Remove();
      return;
    }
  actor->px = actor->py = actor->pz = 0;
  actor->SetState(mobjinfo[actor->type].deathstate);
  actor->flags &= ~MF_MISSILE;
  fog = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_TFOG);
  S_StartSound(fog, sfx_telept);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthInit
//
//----------------------------------------------------------------------------

void A_Sor2DthInit(Actor *actor)
{
  actor->special1 = 7; // Animation loop counter
  actor->mp->Massacre(); // Kill monsters early
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthLoop
//
//----------------------------------------------------------------------------

void A_Sor2DthLoop(Actor *actor)
{
  if(--actor->special1)
    // Need to loop
    actor->SetState(S_SOR2_DIE4);
}

//----------------------------------------------------------------------------
//
// D'Sparil Sound Routines
//
//----------------------------------------------------------------------------

void A_SorZap(Actor *actor) {S_StartAmbSound(sfx_sorzap);}
void A_SorRise(Actor *actor) {S_StartAmbSound(sfx_sorrise);}
void A_SorDSph(Actor *actor) {S_StartAmbSound(sfx_sordsph);}
void A_SorDExp(Actor *actor) {S_StartAmbSound(sfx_sordexp);}
void A_SorDBon(Actor *actor) {S_StartAmbSound(sfx_sordbon);}
void A_SorSightSnd(Actor *actor) {S_StartAmbSound(sfx_sorsit);}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk1
//
// Melee attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk1(Actor *actor)
{
  Actor *t = actor->target;

  if (t == NULL)
    return;
    
  S_StartSound(actor, sfx_stfpow);
  if (actor->CheckMeleeRange())
    {
      t->Damage(actor, actor, HITDICE(4));

      if (t->Type() == Thinker::tt_ppawn) 
	{ // Squish the player
	  ((PlayerPawn*)t)->player->deltaviewheight = -16*FRACUNIT;
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurDecide
//
// Choose a missile attack.
//
//----------------------------------------------------------------------------

#define MNTR_CHARGE_SPEED (13*FRACUNIT)

void A_MinotaurDecide(Actor *actor)
{
  angle_t angle;
  Actor *target;
  int dist;

  target = actor->target;
  if(!target)
    {
      return;
    }
  S_StartSound(actor, sfx_minsit);
  dist = P_AproxDistance(actor->x-target->x, actor->y-target->y);
  if(target->z+target->height > actor->z
     && target->z+target->height < actor->z+actor->height
     && dist < 8*64*FRACUNIT
     && dist > 1*64*FRACUNIT
     && P_Random() < 150)
    { // Charge attack
      // Don't call the state function right away
      actor->SetState(S_MNTR_ATK4_1, false);
      actor->flags |= MF_SKULLFLY;
      A_FaceTarget(actor);
      angle = actor->angle>>ANGLETOFINESHIFT;
      actor->px = FixedMul(MNTR_CHARGE_SPEED, finecosine[angle]);
      actor->py = FixedMul(MNTR_CHARGE_SPEED, finesine[angle]);
      actor->special1 = TICRATE/2; // Charge duration
    }
  else if(target->z == target->floorz
	  && dist < 9*64*FRACUNIT
	  && P_Random() < 220)
    { // Floor fire attack
      actor->SetState(S_MNTR_ATK3_1);
      actor->special2 = 0;
    }
  else
    { // Swing attack
      A_FaceTarget(actor);
      // Don't need to call SetState because the current state
      // falls through to the swing attack
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurCharge
//
//----------------------------------------------------------------------------

void A_MinotaurCharge(Actor *actor)
{
  Actor *puff;

  if(actor->special1)
    {
      puff = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
      puff->pz = 2*FRACUNIT;
      actor->special1--;
    }
  else
    {
      actor->flags &= ~MF_SKULLFLY;
      actor->SetState(actor->info->seestate);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk2
//
// Swing attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk2(Actor *actor)
{
  Actor *mo;
  angle_t angle;
  fixed_t pz;

  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, sfx_minat2);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(5));
      return;
    }
  mo = actor->SpawnMissile(actor->target, MT_MNTRFX1);
  if(mo)
    {
      S_StartSound(mo, sfx_minat2);
      pz = mo->pz;
      angle = mo->angle;
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/8), pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/8), pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/16), pz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/16), pz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk3
//
// Floor fire attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk3(Actor *actor)
{
  Actor *mo;
  Actor *t = actor->target;

  if (t == NULL)
    return;

  if (actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(5));
      if (t->Type() == Thinker::tt_ppawn)
	{ // Squish the player
	  ((PlayerPawn *)t)->player->deltaviewheight = -16*FRACUNIT;
	}
    }
  else
    {
      mo = actor->SpawnMissile(actor->target, MT_MNTRFX2);
      if (mo != NULL)
	{
	  S_StartSound(mo, sfx_minat1);
	}
    }
  if (P_Random() < 192 && actor->special2 == 0)
    {
      actor->SetState(S_MNTR_ATK3_4);
      actor->special2 = 1;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MntrFloorFire
//
//----------------------------------------------------------------------------

void A_MntrFloorFire(Actor *actor)
{
  Actor *mo;
  int r,s;
    
  actor->z = actor->floorz;
  r = P_SignedRandom();
  s = P_SignedRandom();
  mo = actor->mp->SpawnActor(actor->x+(r<<10),
		   actor->y+(s<<10), ONFLOORZ, MT_MNTRFX3);
  mo->target = actor->target;
  mo->px = 1; // Force block checking
  mo->CheckMissileSpawn();
}

//----------------------------------------------------------------------------
//
// PROC A_BeastAttack
//
//----------------------------------------------------------------------------

void A_BeastAttack(Actor *actor)
{
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(3));
      return;
    }
  actor->SpawnMissile(actor->target, MT_BEASTBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadAttack
//
//----------------------------------------------------------------------------

void A_HHeadAttack(Actor *actor)
{
  int i;
  static int atkResolve1[] = { 50, 150 };
  static int atkResolve2[] = { 150, 200 };

  // Ice ball             (close 20% : far 60%)
  // Fire column  (close 40% : far 20%)
  // Whirlwind    (close 40% : far 20%)
  // Distance threshold = 8 cells

  Actor *t = actor->target;
  if (t == NULL)
    return;

  A_FaceTarget(actor);
  if(actor->CheckMeleeRange())
    {
      t->Damage(actor, actor, HITDICE(6));
      return;
    }

  int dist = P_AproxDistance(actor->x - t->x, actor->y - t->y) > 8*64*FRACUNIT;
  int randAttack = P_Random();
  if (randAttack < atkResolve1[dist])
    { // Ice ball
      actor->SpawnMissile(t, MT_HEADFX1);
      S_StartSound(actor, sfx_hedat2);
    }
  else if(randAttack < atkResolve2[dist])
    { // Fire column
      Actor *baseFire = actor->SpawnMissile(t, MT_HEADFX3);
      if (baseFire != NULL)
	{
	  baseFire->SetState(S_HHEADFX3_4); // Don't grow
	  for (i = 0; i < 5; i++)
	    {
	      Actor *fire = actor->mp->SpawnActor(baseFire->x, baseFire->y,
						  baseFire->z, MT_HEADFX3);
	      if (i == 0)
		S_StartSound(actor, sfx_hedat1);

	      fire->owner = baseFire->owner;
	      fire->angle = baseFire->angle;
	      fire->px = baseFire->px;
	      fire->py = baseFire->py;
	      fire->pz = baseFire->pz;
                                //fire->damage = 0;
	      fire->health = (i+1)*2;
	      fire->CheckMissileSpawn();
	    }
	}
    }
  else
    { // Whirlwind
      Actor *mo = actor->SpawnMissile(t, MT_WHIRLWIND);
      if(mo != NULL)
	{
	  mo->z -= 32*FRACUNIT;
	  mo->target = t;
	  mo->special2 = 50; // Timer for active sound
	  mo->health = 20*TICRATE; // Duration
	  S_StartSound(actor, sfx_hedat3);
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_WhirlwindSeek
//
//----------------------------------------------------------------------------

void A_WhirlwindSeek(Actor *actor)
{
  actor->health -= 3;
  if(actor->health < 0)
    {
      actor->px = actor->py = actor->pz = 0;
      actor->SetState(mobjinfo[actor->type].deathstate);
      actor->flags &= ~MF_MISSILE;
      return;
    }
  if((actor->special2 -= 3) < 0)
    {
      actor->special2 = 58+(P_Random()&31);
      S_StartSound(actor, sfx_hedat3);
    }
  if (actor->target && actor->target->flags & MF_SHADOW)
    return;

  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadIceImpact
//
//----------------------------------------------------------------------------

void A_HeadIceImpact(Actor *ice)
{
  int i;
  angle_t angle;
  Actor *shard;

  for(i = 0; i < 8; i++)
    {
      shard = ice->mp->SpawnActor(ice->x, ice->y, ice->z, MT_HEADFX2);
      angle = i*ANG45;
      shard->target = ice->target;
      shard->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      shard->px = FixedMul(shard->info->speed, finecosine[angle]);
      shard->py = FixedMul(shard->info->speed, finesine[angle]);
      shard->pz = -.6*FRACUNIT;
      shard->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HeadFireGrow
//
//----------------------------------------------------------------------------

void A_HeadFireGrow(Actor *fire)
{
  fire->health--;
  fire->z += 9*FRACUNIT;
  if(fire->health == 0)
    {
      //fire->damage = fire->info->damage;
      fire->SetState(S_HHEADFX3_4);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack
//
//----------------------------------------------------------------------------

void A_SnakeAttack(Actor *actor)
{
  if(!actor->target)
    {
      actor->SetState(S_SNAKE_WALK1);
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  actor->SpawnMissile(actor->target, MT_SNAKEPRO_A);
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack2
//
//----------------------------------------------------------------------------

void A_SnakeAttack2(Actor *actor)
{
  if(!actor->target)
    {
      actor->SetState(S_SNAKE_WALK1);
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor);
  actor->SpawnMissile(actor->target, MT_SNAKEPRO_B);
}

//----------------------------------------------------------------------------
//
// PROC A_ClinkAttack
//
//----------------------------------------------------------------------------

void A_ClinkAttack(Actor *actor)
{
  int damage;

  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      damage = ((P_Random()%7)+3);
      actor->target->Damage(actor, actor, damage);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GhostOff
//
//----------------------------------------------------------------------------

void A_GhostOff(Actor *actor)
{
  actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk1
//
//----------------------------------------------------------------------------

void A_WizAtk1(Actor *actor)
{
  A_FaceTarget(actor);
  actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk2
//
//----------------------------------------------------------------------------

void A_WizAtk2(Actor *actor)
{
  A_FaceTarget(actor);
  actor->flags |= MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk3
//
//----------------------------------------------------------------------------

void A_WizAtk3(Actor *actor)
{
  Actor *mo;
  angle_t angle;
  fixed_t pz;

  actor->flags &= ~MF_SHADOW;
  if(!actor->target)
    {
      return;
    }
  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(4));
      return;
    }
  mo = actor->SpawnMissile(actor->target, MT_WIZFX1);
  if(mo)
    {
      pz = mo->pz;
      angle = mo->angle;
      actor->SpawnMissileAngle(MT_WIZFX1, angle-(ANG45/8), pz);
      actor->SpawnMissileAngle(MT_WIZFX1, angle+(ANG45/8), pz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Scream
//
//----------------------------------------------------------------------------

void A_HScream(Actor *actor)
{
  switch(actor->type)
    {
    case MT_CHICPLAYER:
    case MT_SORCERER1:
    case MT_MINOTAUR:
      // Make boss death sounds full volume
      S_StartAmbSound(actor->info->deathsound);
      break;
    case MT_PLAYER:
      // Handle the different player death screams
      if(actor->special1 < 10)
	{ // Wimpy death sound
	  S_StartSound(actor, sfx_plrwdth);
	}
      else if(actor->health > -50)
	{ // Normal death sound
	  S_StartSound(actor, actor->info->deathsound);
	}
      else if(actor->health > -100)
	{ // Crazy death sound
	  S_StartSound(actor, sfx_plrcdth);
	}
      else
	{ // Extreme death sound
	  S_StartSound(actor, sfx_gibdth);
	}
      break;
    default:
      S_StartSound(actor, actor->info->deathsound);
      break;
    }
}

//---------------------------------------------------------------------------
//
// PROC P_DropItem
//
//---------------------------------------------------------------------------

void P_DropItem(Actor *source, mobjtype_t type, int special, int chance)
{
  Actor *mo;

  if(P_Random() > chance)
    {
      return;
    }
  mo = source->mp->SpawnActor(source->x, source->y,
		   source->z+(source->height>>1), type);
  mo->px = P_SignedRandom()<<8;
  mo->py = P_SignedRandom()<<8;
  mo->pz = FRACUNIT*5+(P_Random()<<10);
  mo->flags |= MF_DROPPED;
  mo->health = special;
}

//----------------------------------------------------------------------------
//
// PROC A_NoBlocking
//
//----------------------------------------------------------------------------

void A_NoBlocking(Actor *actor)
{
  A_Fall(actor);
  // Check for monsters dropping things
  switch(actor->type)
    {
    case MT_MUMMY:
    case MT_MUMMYLEADER:
    case MT_MUMMYGHOST:
    case MT_MUMMYLEADERGHOST:
      P_DropItem(actor, MT_AMGWNDWIMPY, 3, 84);
      break;
    case MT_KNIGHT:
    case MT_KNIGHTGHOST:
      P_DropItem(actor, MT_AMCBOWWIMPY, 5, 84);
      break;
    case MT_WIZARD:
      P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
      P_DropItem(actor, MT_ARTITOMEOFPOWER, 0, 4);
      break;
    case MT_HHEAD:
      P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
      P_DropItem(actor, MT_ARTIEGG, 0, 51);
      break;
    case MT_BEAST:
      P_DropItem(actor, MT_AMCBOWWIMPY, 10, 84);
      break;
    case MT_CLINK:
      P_DropItem(actor, MT_AMSKRDWIMPY, 20, 84);
      break;
    case MT_SNAKE:
      P_DropItem(actor, MT_AMPHRDWIMPY, 5, 84);
      break;
    case MT_MINOTAUR:
      P_DropItem(actor, MT_ARTISUPERHEAL, 0, 51);
      P_DropItem(actor, MT_AMPHRDWIMPY, 10, 84);
      break;
    default:
      break;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_PodPain
//
//----------------------------------------------------------------------------

void A_PodPain(Actor *actor)
{
  int i;
  int count;
  int chance;
  Actor *goo;
    
  chance = P_Random();
  if(chance < 128)
    {
      return;
    }
  count = chance > 240 ? 2 : 1;
  for(i = 0; i < count; i++)
    {
      goo = actor->mp->SpawnActor(actor->x, actor->y,
			actor->z+48*FRACUNIT, MT_PODGOO);
      goo->target = actor;
      goo->px = P_SignedRandom()<<9;
      goo->py = P_SignedRandom()<<9;
      goo->pz = FRACUNIT/2+(P_Random()<<9);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_RemovePod
//
//----------------------------------------------------------------------------

void A_RemovePod(Actor *actor)
{
  Actor *mo;

  if(actor->special2)
    {
      mo = (Actor *)actor->special2;
      if(mo->special1 > 0)
	{
	  mo->special1--;
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MakePod
//
//----------------------------------------------------------------------------

#define MAX_GEN_PODS 16

void A_MakePod(Actor *actor)
{
  if (actor->special1 == MAX_GEN_PODS)
    // Too many generated pods
    return;

  fixed_t  x, y, z;
    
  x = actor->x;
  y = actor->y;
  z = actor->z;
  Actor *mo = actor->mp->SpawnActor(x, y, ONFLOORZ, MT_POD);
  if (mo->CheckPosition(x, y) == false)
    { // Didn't fit
      mo->Remove();
      return;
    }
  mo->SetState(S_POD_GROW1);
  mo->Thrust(P_Random()<<24, (fixed_t)(4.5*FRACUNIT));
  S_StartSound(mo, sfx_newpod);
  actor->special1++; // Increment generated pod count
  mo->special2 = (int)actor; // Link the generator to the pod (nice code, Raven;)
  return;
}


//----------------------------------------------------------------------------
//
// PROC A_BossDeath
//
// Trigger special effects if all bosses are dead.
//
//----------------------------------------------------------------------------

void A_HBossDeath(Actor *actor)
{
  actor->mp->BossDeath(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ESound
//
//----------------------------------------------------------------------------

void A_ESound(Actor *mo)
{
  int sound;

  switch(mo->type)
    {
    case MT_SOUNDWATERFALL:
      sound = sfx_waterfl;
      break;
    case MT_SOUNDWIND:
      sound = sfx_wind;
      break;
    default:
      sound = sfx_None;
      break;
    }
  S_StartSound(mo, sound);
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter(Actor *actor)
{
  Actor *mo;
  int r,s;

  r = P_Random();
  s = P_Random();
  mo = actor->mp->SpawnActor(actor->x+((r&31)-16)*FRACUNIT,
		   actor->y+((s&31)-16)*FRACUNIT,
		   actor->subsector->sector->floorheight, MT_TELEGLITTER);
  mo->pz = FRACUNIT/4;
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter2
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter2(Actor *actor)
{
  Actor *mo;
  int r,s;

  r = P_Random();
  s = P_Random();
  mo = actor->mp->SpawnActor(actor->x+((r&31)-16)*FRACUNIT,
		   actor->y+((s&31)-16)*FRACUNIT,
		   actor->subsector->sector->floorheight, MT_TELEGLITTER2);
  mo->pz = FRACUNIT/4;
}

//----------------------------------------------------------------------------
//
// PROC A_AccTeleGlitter
//
//----------------------------------------------------------------------------

void A_AccTeleGlitter(Actor *actor)
{
  if(++actor->health > 35)
    {
      actor->pz += actor->pz/2;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_InitKeyGizmo
//
//----------------------------------------------------------------------------

void A_InitKeyGizmo(Actor *gizmo)
{
  Actor *mo;
  statenum_t state;

  switch(gizmo->type)
    {
    case MT_KEYGIZMOBLUE:
      state = S_KGZ_BLUEFLOAT1;
      break;
    case MT_KEYGIZMOGREEN:
      state = S_KGZ_GREENFLOAT1;
      break;
    case MT_KEYGIZMOYELLOW:
      state = S_KGZ_YELLOWFLOAT1;
      break;
    default:
      state = S_NULL;
      break;
    }
  mo = gizmo->mp->SpawnActor(gizmo->x, gizmo->y, gizmo->z+60*FRACUNIT,
			     MT_KEYGIZMOFLOAT);
  mo->SetState(state);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoSet
//
//----------------------------------------------------------------------------

void A_VolcanoSet(Actor *volcano)
{
  volcano->tics = 105+(P_Random()&127);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoBlast
//
//----------------------------------------------------------------------------

void A_VolcanoBlast(Actor *volcano)
{
  int i;
  int count;
  angle_t angle;

  count = 1+(P_Random()%3);
  for(i = 0; i < count; i++)
    {
      Actor *blast = volcano->mp->SpawnActor(volcano->x, volcano->y,
	volcano->z+44*FRACUNIT, MT_VOLCANOBLAST); // MT_VOLCANOBLAST
      //blast->target = volcano;
      blast->owner = volcano;
      angle = P_Random()<<24;
      blast->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      blast->px = FixedMul(1*FRACUNIT, finecosine[angle]);
      blast->py = FixedMul(1*FRACUNIT, finesine[angle]);
      blast->pz = (2.5*FRACUNIT)+(P_Random()<<10);
      S_StartSound(blast, sfx_volsht);
      blast->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_VolcBallImpact
//
//----------------------------------------------------------------------------

void A_VolcBallImpact(Actor *ball)
{
  int i;
  Actor *tiny;
  angle_t angle;

  if(ball->z <= ball->floorz)
    {
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~MF2_LOGRAV;
      ball->z += 28*FRACUNIT;
      //ball->pz = 3*FRACUNIT;
    }
  ball->RadiusAttack(ball->owner, 25);
  for(i = 0; i < 4; i++)
    {
      tiny = ball->mp->SpawnActor(ball->x, ball->y, ball->z, MT_VOLCANOTBLAST);
      tiny->owner = ball;
      angle = i*ANG90;
      tiny->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->px = FixedMul(FRACUNIT*.7, finecosine[angle]);
      tiny->py = FixedMul(FRACUNIT*.7, finesine[angle]);
      tiny->pz = FRACUNIT+(P_Random()<<9);
      tiny->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SkullPop
//
//----------------------------------------------------------------------------
/*
void A_SkullPop(Actor *actor)
{
  player_t *player;

  actor->flags &= ~MF_SOLID;
  Actor *mo = actor->mp->SpawnActor(actor->x, actor->y, actor->z+48*FRACUNIT,
			  MT_BLOODYSKULL);
  //mo->target = actor;
  mo->px = P_SignedRandom()<<9;
  mo->py = P_SignedRandom()<<9;
  mo->pz = FRACUNIT*2+(P_Random()<<6);
  // Attach player mobj to bloody skull
  player = actor->player;
  actor->player = NULL;
  mo->player = player;
  mo->health = actor->health;
  mo->angle = actor->angle;
  player->mo = mo;
  player->aiming = 0;
  player->damagecount = 32;
}
*/
//----------------------------------------------------------------------------
//
// PROC A_CheckSkullFloor
//
//----------------------------------------------------------------------------

void A_CheckSkullFloor(Actor *actor)
{
  if(actor->z <= actor->floorz)
    {
      actor->SetState(S_BLOODYSKULLX1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckSkullDone
//
//----------------------------------------------------------------------------

void A_CheckSkullDone(Actor *actor)
{
  if(actor->special2 == 666)
    {
      actor->SetState(S_BLOODYSKULLX2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckBurnGone
//
//----------------------------------------------------------------------------

void A_CheckBurnGone(Actor *actor)
{
  if(actor->special2 == 666)
    {
      //actor->SetState(S_PLAY_FDTH20);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FreeTargMobj
//
//----------------------------------------------------------------------------

void A_FreeTargMobj(Actor *mo)
{
  mo->px = mo->py = mo->pz = 0;
  mo->z = mo->ceilingz+4*FRACUNIT;
  mo->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY|MF_SOLID);
  mo->flags |= MF_CORPSE|MF_DROPOFF|MF_NOGRAVITY;
  mo->flags2 &= ~(MF2_PASSMOBJ|MF2_LOGRAV);
  //mo->player = NULL;
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerCorpse
//
//----------------------------------------------------------------------------

void A_AddPlayerCorpse(Actor *actor)
{
  Map *m = actor->mp;

  if (m->bodyqueue.size() >= BODYQUESIZE)
    {
      // Too many player corpses - remove an old one
      (m->bodyqueue.front())->Remove();
      m->bodyqueue.pop_front();
    }
  m->bodyqueue.push_back(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_FlameSnd
//
//----------------------------------------------------------------------------

void A_FlameSnd(Actor *actor)
{
  S_StartSound(actor, sfx_hedat1); // Burn sound
}