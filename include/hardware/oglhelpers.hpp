// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006 by DooM Legacy Team.
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

// This file pair contains all sorts of helper functions and arrays
// used by the OpenGL renderer.

#ifndef OGLHELPERS_CPP
#define OGLHELPERS_CPP

#include"doomtype.h"

extern GLfloat viewport_multipliers[4][4][4];

byte LightLevelToLum(int l, int extralight=0);
void InitLumLut();
bool GLExtAvailable(char *extension);

#endif // OGLHELPERS_CPP
