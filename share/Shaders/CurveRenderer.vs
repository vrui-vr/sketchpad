/***********************************************************************
CurveTest.vs - Vertex shader for anti-aliased curve rendering.
Copyright (c) 2024-2025 Oliver Kreylos

This file is part of the SketchPad vector drawing package.

The SketchPad vector drawing package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The SketchPad vector drawing package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the SketchPad vector drawing package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

varying vec2 vNormal;

void main()
	{
	/* Pass vertex color, normal, and model-space position to geometry shader: */
	gl_FrontColor=gl_Color;
	vNormal=gl_Normal.xy;
	gl_Position=gl_Vertex;
	}
