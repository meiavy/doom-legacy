# Gnu Make makefile for Doom Legacy / SDL / GCC
# Ville Bergholm 2002
# This primary Makefile calls auxiliary Makefiles in subdirectories
#
# Use OPT=1 to make optimized version, else you'll get debug info.

# Platform autodetect.

UNAME = $(shell uname)

ifeq ($(UNAME),Linux)
export LINUX=1
else
export WIN=1
endif

# Debugging info

ifdef OPT
DEBUGFLAGS = 
OPTFLAGS = -O
else
DEBUGFLAGS = -g 
OPTFLAGS = 
endif

# ----------- platform specific part begins
ifdef LINUX

# file removal utility
 rm = rm
# assembler
 NASM = nasm
 nasmformat = elf - DLINUX # hmmm... a define here...
# compiler
 platform = -DLINUX
 interface = -DSDL
# linker
 LIBS	= -L/usr/X11/lib -lSDLmain -lSDL -lSDL_mixer -lGL -lGLU
# -lm -lpthread ???
 LDFLAGS = -Wall
# executable
 exename = Legacy

else # assume WIN32 is defined

# file removal utility
 rm = rm
# assembler
 NASM 	= nasmw.exe
 nasmformat = win32
# compiler
 platform = -D__WIN32__
 interface = -DSDL
 EXTRAFLAGS = -mwindows
# linker
 LIBS	= -lmingw32 -lSDLmain -lSDL SDL_mixer.lib -lopengl32 -lglu32 -lwsock32
 LDFLAGS = -Wall -mwindows
# executable
 exename = Legacy.exe

endif
# ----------- platform specific part ends

export rm

# C++ compiler (usually g++)
export CC = g++

# Defines. This is what they mean:
# Automatic defines: Automatically defined by the compiler in the corresponding environment. No need to define.
# __WIN32__, __WIN32, _WIN32, WIN32 : defined in Win32 environment
# __DJGPP__ : defined by DJGPP
# _MACOS_ : ???
# __OS2__ : ???
#
# Platform: use _exactly_ one of the following:
# LINUX : Linux platform
# __WIN32__ : Win32 platform (automatic, no need to define)
# PC_DOS : DJGPP version of Legacy for MS-DOS.
#
# Multimedia interface: use ONLY _one_ of the following:
# SDL : we are compiling the SDL version of Legacy (use SDL for multimedia interface, SDL_mixer for music)
# WIN32_DIRECTX : we are compiling the Win32 native version of Legacy. Use DirectX for multimedia interface
# LINUX_X : Linux "native" with X11 video etc.
# VID_X11 : X11 / glX hardware rendering, use with LINUX_X
#
# Miscellaneous options: use as many as you like
# USEASM : use assembler routines where possible
# HWRENDER : compile with hardware renderer included
# HW3SOUND : compile with hardware 3D sound included. Currently only for DirectX.
# DIRECTFULLSCREEN : allow change into fullscreen mode through console????
# NDEBUG : catch various signals?
# DEBUG : renderer and hud debugging messages?
# POLL_POINTER : Something to do with linux_x version?
#
# ??? PURESDL, 
# hmm. In Win98, -DUSEASM causes execution to stop (ASM_PatchRowBytes())

defines := $(platform) $(interface) -DNDEBUG -DHWRENDER -DHW3SOUND -DDIRECTFULLSCREEN
export CF := $(DEBUGFLAGS) $(OPTFLAGS) -Wall $(EXTRAFLAGS) $(defines) #-ansi
INCLUDES = -Iinclude
CFLAGS = $(CF) $(INCLUDES)

# linker
LD     	= $(CC)

export objdir = objs

export engine_objects = \
	$(objdir)/m_menu.o \
	$(objdir)/f_finale.o \
	$(objdir)/f_wipe.o \
	$(objdir)/wi_stuff.o \
	$(objdir)/am_map.o \
	$(objdir)/hu_stuff.o \
	$(objdir)/st_lib.o \
	$(objdir)/st_stuff.o \
	$(objdir)/g_player.o \
	$(objdir)/g_game.o \
	$(objdir)/g_state.o \
	$(objdir)/g_demo.o \
	$(objdir)/g_map.o \
	$(objdir)/g_think.o \
	$(objdir)/g_actor.o \
	$(objdir)/g_pawn.o \
	$(objdir)/g_input.o \
	$(objdir)/d_main.o \
	$(objdir)/m_cheat.o \
	$(objdir)/p_genlin.o \
	$(objdir)/p_ceilng.o \
	$(objdir)/p_doors.o \
	$(objdir)/p_floor.o \
	$(objdir)/p_lights.o \
	$(objdir)/p_plats.o \
	$(objdir)/p_telept.o \
	$(objdir)/p_switch.o \
	$(objdir)/p_spec.o \
	$(objdir)/p_enemy.o \
	$(objdir)/p_henemy.o \
	$(objdir)/p_heretic.o \
	$(objdir)/p_camera.o \
	$(objdir)/p_user.o \
	$(objdir)/p_pspr.o \
	$(objdir)/p_hpspr.o \
	$(objdir)/p_tick.o \
	$(objdir)/p_sight.o \
	$(objdir)/p_hsight.o \
	$(objdir)/p_info.o \
	$(objdir)/p_setup.o \
	$(objdir)/p_saveg.o \
	$(objdir)/p_inter.o \
	$(objdir)/p_maputl.o \
	$(objdir)/p_map.o \
	$(objdir)/p_fab.o \
	$(objdir)/d_items.o \
	$(objdir)/dstrings.o \
	$(objdir)/info.o
#	$(objdir)/t_func.o \
#	$(objdir)/t_oper.o \
#	$(objdir)/t_parse.o \
#	$(objdir)/t_prepro.o \
#	$(objdir)/t_script.o \
#	$(objdir)/t_spec.o \
#	$(objdir)/t_vari.o \


export util_objects = \
	$(objdir)/command.o \
	$(objdir)/console.o \
	$(objdir)/dehacked.o \
	$(objdir)/m_argv.o \
	$(objdir)/m_bbox.o \
	$(objdir)/m_fixed.o \
	$(objdir)/m_misc.o \
	$(objdir)/m_random.o \
	$(objdir)/m_swap.o \
	$(objdir)/md5.o \
	$(objdir)/tables.o \
	$(objdir)/wad.o \
	$(objdir)/w_wad.o \
	$(objdir)/z_zone.o

export audio_objects = \
	$(objdir)/hw3sound.o \
	$(objdir)/qmus2mid.o \
	$(objdir)/s_amb.o \
	$(objdir)/s_sound.o \
	$(objdir)/sounds.o

export video_objects = \
	$(objdir)/screen.o \
	$(objdir)/v_video.o \
	$(objdir)/r_render.o \
	$(objdir)/r_bsp.o \
	$(objdir)/r_data.o \
	$(objdir)/r_draw.o \
	$(objdir)/r_draw8.o \
	$(objdir)/r_draw16.o \
	$(objdir)/r_main.o \
	$(objdir)/r_plane.o \
	$(objdir)/r_segs.o \
	$(objdir)/r_sky.o \
	$(objdir)/r_splats.o \
	$(objdir)/r_things.o \
	$(objdir)/hw_bsp.o \
	$(objdir)/hw_cache.o \
	$(objdir)/hw_draw.o \
	$(objdir)/hw_light.o \
	$(objdir)/hw_main.o \
	$(objdir)/hw_md2.o \
	$(objdir)/hw_trick.o

export net_objects = \
	$(objdir)/d_client.o \
	$(objdir)/d_server.o \
	$(objdir)/d_net.o \
	$(objdir)/d_netcmd.o \
	$(objdir)/d_netfil.o \
	$(objdir)/i_tcp.o \
	$(objdir)/mserv.o

export sdl_objects = \
	$(objdir)/dosstr.o \
	$(objdir)/endtxt.o \
	$(objdir)/filesrch.o \
	$(objdir)/i_cdmus.o \
	$(objdir)/i_main.o \
	$(objdir)/i_net.o \
	$(objdir)/i_sound.o \
	$(objdir)/i_system.o \
	$(objdir)/i_video.o \
	$(objdir)/ogl_sdl.o \
	$(objdir)/searchp.o

#	$(objdir)/hwsym_sdl.o \

asm_objects = $(objdir)/tmap.o
# not used at the moment


objects = $(engine_objects) $(util_objects) $(audio_objects) $(video_objects) \
	$(net_objects) $(sdl_objects) $(objdir)/r_opengl.o # $(asm_objects)
# note the r_opengl.o here!

all	: $(exename)

.PHONY	: clean depend engine util audio video net sdl

clean	:
	$(rm) $(objects)

depend:
	$(MAKE) -C video depend
#	$(MAKE) -C engine depend
#	$(MAKE) -C util depend
#	$(MAKE) -C audio depend
#	$(MAKE) -C net depend
#	$(MAKE) -C interface/sdl depend

engine	:
	$(MAKE) -C engine

util	:
	$(MAKE) -C util

audio	:
	$(MAKE) -C audio

video	:
	$(MAKE) -C video

net	:
	$(MAKE) -C net

sdl	:
	$(MAKE) -C interface/sdl


# explicit rules

$(exename) : engine util audio video net sdl $(objdir)/r_opengl.o
	$(LD) $(LDFLAGS) $(objects) $(LIBS) -o $@

$(objdir)/r_opengl.o : video/hardware/r_opengl/r_opengl.cpp include/hardware/r_opengl/r_opengl.h
	$(CC) -c $(CFLAGS) $< -o $@

# this isn't used now
$(objdir)/tmap.o : assembler/tmap.nas
	$(NASM) -o $@ -f $(nasmformat) $<

# this may be useless
$(objdir)/vid_copy.o: assembler/vid_copy.s
	sentinel_nonsense_name $(CC) $(OPTS) $(SFLAGS) -x assembler-with-cpp -c $< -o $@