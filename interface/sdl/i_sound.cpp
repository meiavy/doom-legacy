// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2000-2005 by Doom Legacy team
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log$
// Revision 1.29  2006/01/02 17:02:31  smite-meister
// small fixes
//
// Revision 1.28  2005/11/06 19:36:53  smite-meister
// ntexture
//
// Revision 1.27  2005/05/29 11:30:42  segabor
// Fixed __APPLE directive__ to __APPLE_CC__ on Mac OS X, new 'Doom Legacy' Xcode project target
//
// Revision 1.24  2005/03/19 13:51:30  smite-meister
// sound samplerate fix
//
// Revision 1.23  2005/03/16 21:16:08  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.22  2005/03/04 16:23:08  smite-meister
// mp3, sector_t
//
// Revision 1.21  2005/01/25 18:29:16  smite-meister
// preparing for alpha
//
// Revision 1.20  2004/12/09 06:16:16  segabor
// fixed SDL headers for Mac
//
// Revision 1.18  2004/08/18 14:35:21  smite-meister
// PNG support!
//
// Revision 1.17  2004/07/13 20:23:38  smite-meister
// Mod system basics
//
// Revision 1.15  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.14  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.12  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.11  2003/12/21 18:35:15  jussip
// Minor cleanup.
//
// Revision 1.10  2003/04/24 00:25:43  hurdler
// Ok, since it doesn't work otherwise, add an ifdef
//
// Revision 1.9  2003/04/24 00:03:02  hurdler
// Should fix compiling problem
//
// Revision 1.8  2003/04/23 21:12:02  hurdler
// Do it again more properly
//
// Revision 1.7  2003/04/23 21:02:00  hurdler
// no more linking warning
//
// Revision 1.6  2003/04/14 08:58:31  smite-meister
// Hexen maps load.
//
// Revision 1.5  2003/03/08 16:07:18  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.3  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.2  2002/12/23 23:25:53  smite-meister
// Ogg Vorbis works!
//
// Revision 1.1.1.1  2002/11/16 14:18:31  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief SDL interface for sound and music.

#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#define USE_RWOPS // for SDL_mixer 1.2.6

#if defined(FREEBSD) || defined(__APPLE_CC__) || defined(__MACOS__)
# include <SDL.h>
# include <SDL_mixer.h>
#else
# include <SDL/SDL.h>
# include <SDL/SDL_mixer.h>
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"
#include "cvars.h"

#include "hardware/hw3sound.h"
#include "z_zone.h"

#include "m_fixed.h"
#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"
#include "sounds.h"

#include "d_main.h"

#include "qmus2mid.h"


#define MIDBUFFERSIZE   128*1024
#define SAMPLERATE      22050   // Hz
#define SAMPLECOUNT     512 // requested audio buffer size (about 46 ms at 11 kHz)

class chan_t
{
public:
  // the corresponding SoundSystem channel
  channel_t *ch;

  // The channel data pointers, start and end.
  Uint8 *data;
  Uint8 *end;

  // pitch and samplerate fused together
  fixed_t step;          // The channel step amount...
  fixed_t stepremainder; // ... and a 0.16 bit remainder of last step.

  // Hardware left and right channel volume lookup.
  int *leftvol_lookup;
  int *rightvol_lookup;

  // calculate sound parameters using ch
  void CalculateParams();
};



static vector<chan_t> channels;

// Pitch to stepping lookup in 16.16 fixed point. 64 pitch units = 1 octave
//  0 = 0.25x, 128 = 1x, 256 = 4x
static fixed_t steptable[256];

// Volume lookups.
static int vol_lookup[128][256];

// Buffer for MIDI
static byte *mus2mid_buffer;


// Flags for the -nosound and -nomusic options
extern bool nosound;
extern bool nomusic;

static bool musicStarted = false;
static bool soundStarted = false;

static SDL_AudioSpec audio;




static void I_SetChannels()
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int i, j;

  // This table provides step widths for pitch parameters.
  for (i = 0; i < 256; i++)
    steptable[i] = float(pow(2.0, ((i-128)/64.0)));

  // Generates volume lookup tables
  //  which also turn the U8 samples
  //  into S16 samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i][j] = (i*(j-128)*256)/127;
}

/*
static int FindChannel(int handle)
{
  int i;

  for (i = 0; i < NUM_CHANNELS; i++)
    if (channels[i].handle == handle)
      return i;
  
  // not found
  return -1;
}
*/


//----------------------------------------------


void I_SetSfxVolume(int volume)
{
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  // Since the variable is cv_soundvolume,
  // nothing needs to be done.
}

// used to (re)calculate channel params
void chan_t::CalculateParams()
{
  // how fast should the sound sample be played?
  step = (double(ch->si->rate) / audio.freq) * steptable[ch->opitch];

  // x^2 separation, that is, orientation/stereo.
  //  range is: 0 (left) - 255 (right)

  // Volume arrives in range 0..255 and it must be in 0..cv_soundvolume...
  int vol = (ch->ovol * cv_soundvolume.value) >> 7;
  // note: >> 6 would use almost the entire dynamical range, but
  // then there would be no "dynamical room" for other sounds :-/
  int sep = ch->osep;

  int leftvol  = vol - ((vol*sep*sep) >> 16); ///(256*256);
  sep = 255 - sep;
  int rightvol = vol - ((vol*sep*sep) >> 16);

  // Sanity check, clamp volume.
  if (rightvol < 0 || rightvol > 127)
    I_Error("rightvol out of bounds");

  if (leftvol < 0 || leftvol > 127)
    I_Error("leftvol out of bounds");

  // Get the proper lookup table piece
  //  for this volume level???
  leftvol_lookup = vol_lookup[leftvol];
  rightvol_lookup = vol_lookup[rightvol];
}

//----------------------------------------------
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.

int I_StartSound(channel_t *s_channel)
{
  if (nosound)
    return 0;

#ifdef NOMIXER
  SDL_LockAudio();
#endif

  int n = channels.size();
  //CONS_Printf("SDL channels: %d\n", n);

  // Tales from the cryptic.
  // Because SoundSystem class takes care of channel management,
  // we know that a new channel can be created.
  channels.resize(n+1);
  chan_t *c = &channels[n];
  c->ch = s_channel;

  // Set pointer to raw data.
  c->data = (Uint8 *)s_channel->si->sdata;
  // Set pointer to end of raw data.
  c->end = c->data + s_channel->si->length;
  
  c->stepremainder = 0;

  // Set pitch, volume and separation
  c->CalculateParams();

  s_channel->playing = true;

#ifdef NOMIXER
  SDL_UnlockAudio();
#endif

  /*
  // Assign current handle number.
  static unsigned short handlenum = 100;
  c->handle = handlenum--;

  // Reset current handle number, limited to 1..100.
  if (!handlenum)
    handlenum = 100;

  return c->handle;
  */
  return 1;
}


void I_StopSound(channel_t *c)
{
  vector<chan_t>::iterator i;

  for (i = channels.begin(); i != channels.end(); i++)
    if (i->ch == c)
      break;
 
  if (i != channels.end())
    channels.erase(i);
}

/*
bool I_SoundIsPlaying(int handle)
{
  return (FindChannel(handle) < 0);
}
*/

// Not used by SDL version
//void I_SubmitSound() {}

// Not used by SDL version. Nevertheless, please 
// witness the wit below:
// void I_UpdateSound () {}

/* Pour une raison que j'ignore, la version SDL n'appelle jamais
   ce truc directement. Fonction vide pour garder une compatibilité
   avec le point de vue de legacy... */

// Himmel, Arsch und Zwirn

//
// The SDL audio callback.
//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//
static void I_UpdateSound_sdl(void *unused, Uint8 *stream, int len)
{
  if (nosound)
    return;

  // stream contains len bytes of outgoing stereo (LR order) music data.
  // Here we mix in current sound data.

  // Data, from raw sound, for right and left.
  register unsigned int sample;
  register int dl, dr;

  // Left and right channel
  // are in S16 audio stream, alternating.
  Sint16 *leftout = (Sint16 *)stream;
  Sint16 *rightout = ((Sint16 *)stream) + 1;

  // Step in stream, left and right, thus two.
  int step = 2;

  // Determine end, for left channel only
  //  (right channel is implicit).
  Sint16 *leftend = leftout + len/step;

  // Mix sounds into the mixing buffer.
  // Loop over entire buffer
  while (leftout != leftend)
    {
      // Reset left/right value.
      dl = *leftout;
      dr = *rightout;

      // Love thy L2 chache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      int i, n = channels.size();
      for (i = 0; i < n; i++)
        {
	  chan_t *c = &channels[i];
	  // Check if channel is active.
	  if (c->data)
            {
	      // Get the raw data from the channel.
	      sample = *(c->data);
	      // Add left and right part for this channel (sound)
	      //  to the current data.
	      // Adjust volume accordingly.
	      dl += c->leftvol_lookup[sample];
	      dr += c->rightvol_lookup[sample];
	      // pitch.
	      c->stepremainder += c->step;
	      // 16.16 fixed point: high word is the current stride
	      c->data += c->stepremainder.floor();
	      // cut away high word
	      c->stepremainder = c->stepremainder.frac();
	      
	      // Check whether data is exhausted.
	      if (c->data >= c->end)
		{
		  c->data = NULL;
		  c->ch->playing = false; // notify SoundSystem
		}
            }
        }

      // Clamp to range. Left hardware channel.
      if (dl > 0x7fff)
	*leftout = 0x7fff;
      else if (dl < -0x8000)
	*leftout = -0x8000;
      else
	*leftout = dl;

      // Same for right hardware channel.
      if (dr > 0x7fff)
	*rightout = 0x7fff;
      else if (dr < -0x8000)
	*rightout = -0x8000;
      else
	*rightout = dr;

      // Increment current pointers in stream
      leftout += step;
      rightout += step;
    }
}

/*
bool I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  int i = FindChannel(handle);
  if (i < 0)
    return false;

  chan_t *c = &channels[i];
  if (c->data)
    {
      I_SetChannelParams(c, vol, sep, pitch);
      return true;
    }
  return false;
}
*/




void I_StartupSound()
{
  if (nosound)
    return;

  // Configure sound device
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      CONS_Printf(" Couldn't initialize SDL Audio: %s\n", SDL_GetError());
      nosound = true;
      return;
    }

  // audio device parameters
  audio.freq = SAMPLERATE;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio.format = AUDIO_S16MSB;
#else
  audio.format = AUDIO_S16LSB;
#endif
  audio.channels = 2;
  audio.samples = SAMPLECOUNT;
  audio.callback = I_UpdateSound_sdl;

  I_SetChannels();

  // SDL_mixer controls the audio device, see I_InitMusic.
#ifdef NOMIXER
  SDL_OpenAudio(&audio, NULL);
  CONS_Printf(" Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
  SDL_PauseAudio(0);
  soundStarted = true;
#endif
}




// initializes both sound and music
void I_InitMusic()
{
  if (nosound)
    {
      // FIXME: workaround for shitty programming undoc'ed features
      nomusic = true;
      return;
    }

#ifdef NOMIXER
  nomusic = true;
#else
  // because we use SDL_mixer, audio is opened here.
  if (Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0)
    {
      CONS_Printf(" Unable to open audio: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  int temp; // aargh!
  if (!Mix_QuerySpec(&audio.freq, &audio.format, &temp))
    {
      CONS_Printf(" Mix_QuerySpec: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  Mix_SetPostMix(audio.callback, NULL);  // after mixing music, add sound effects
  CONS_Printf(" Audio device initialized: %d Hz, %d samples/slice.\n", audio.freq, audio.samples);
  Mix_Resume(-1); // start all sound channels (although they are not used)

  soundStarted = true;

  if (nomusic)
    return;

  Mix_ResumeMusic();  // start music playback
  mus2mid_buffer = (byte *)Z_Malloc(MIDBUFFERSIZE, PU_MUSIC, NULL); // FIXME: catch return value
  CONS_Printf(" Music initialized.\n");
  musicStarted = true;
#endif
}



// finish sound and music
void I_ShutdownSound()
{
  if (!soundStarted)
    return;
    
  CONS_Printf("I_ShutdownSound: ");

#ifdef NOMIXER
  SDL_CloseAudio();
#else
  Mix_CloseAudio();
#endif

  CONS_Printf("shut down\n");
  soundStarted = false;

  if (!musicStarted)
    return;

  Z_Free(mus2mid_buffer);
  CONS_Printf("I_ShutdownMusic: shut down\n");
  musicStarted = false;  
}


/// the "registered" piece of music
static struct music_channel_t
{
  Mix_Music *mus;
  SDL_RWops *rwop; ///< must not be freed before music is halted

  music_channel_t() { mus = NULL; rwop = NULL; }
} music;


/// starts playing the "registered" music
void I_PlaySong(int handle, int looping)
{
#ifndef NOMIXER
  if (nomusic)
    return;

  if (music.mus)
    {
      Mix_FadeInMusic(music.mus, looping ? -1 : 0, 500);
    }
#endif
}

void I_PauseSong(int handle)
{
  if (nomusic)
    return;

  I_StopSong(handle);
}

void I_ResumeSong(int handle)
{
  if (nomusic)
    return;

  I_PlaySong(handle, true);
}

void I_StopSong(int handle)
{
#ifndef NOMIXER
  if (nomusic)
    return;

  Mix_FadeOutMusic(500);
#endif
}


void I_UnRegisterSong(int handle)
{
#ifndef NOMIXER
  if (nomusic)
    return;

  if (music.mus)
    {
      Mix_FreeMusic(music.mus);
      SDL_FreeRW(music.rwop);
      music.mus = NULL;
      music.rwop = NULL;
    }
#endif
}


int I_RegisterSong(void* data, int len)
{
#ifndef NOMIXER
  if (nomusic)
    return 0;

  if (music.mus)
    {
      I_Error("Two registered pieces of music simultaneously!\n");
    }

  byte *bdata = static_cast<byte *>(data);

  if (memcmp(data, MUSMAGIC, 4) == 0)
    {
      int err;
      Uint32 midlength;
      // convert mus to mid in memory with a wonderful function
      // thanks to S.Bacquet for the source of qmus2mid
      if ((err = qmus2mid(bdata, mus2mid_buffer, 89, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
	{
	  CONS_Printf("Cannot convert MUS to MIDI: error %d.\n", err);
	  return 0;
	}

      music.rwop = SDL_RWFromConstMem(mus2mid_buffer, midlength);
    }
  else
  //  else if (memcmp(data,"MThd", 4) == 0 || memcmp(data, "OggS", 4) == 0 ||
  // (memcmp(data,"ID3", 3) == 0 || (bdata[0] == 255 && (bdata[1] & 0xe0 == 0xe0))))
    // Damn, MP3 has no real header! This code only recognizes ID3 tags
    // or the 11 bits long frame sync block which is all ones!
    {
      // MIDI, MP3 and Ogg Vorbis
      music.rwop = SDL_RWFromConstMem(data, len);
    }
  /*
  else
    {
      CONS_Printf("Music lump is not in MUS, Midi, MP3 or Ogg Vorbis format!\n");
      return 0;
    }
  */
  
  // FIXME TODO SDL_mixer _should_ support playing also mp3 music directly from memory,
  // I hope the SDL_mixer guys finish this soon!

  music.mus = Mix_LoadMUS_RW(music.rwop);
  if (!music.mus)
    {
      CONS_Printf("Couldn't load music lump: %s\n", Mix_GetError());
      SDL_FreeRW(music.rwop);
      music.rwop = NULL;
    }

#endif

  return 0;
}

void I_SetMusicVolume(int volume)
{
#ifndef NOMIXER
  if (nomusic)
    return;

  // acceptable volume range : 0-128
  Mix_VolumeMusic(volume*2);
#endif
}
