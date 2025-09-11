/***********************************************************************
CurveTest.fs - Fragment shader for anti-aliased curve rendering.
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

uniform float lineWidth;
uniform float pixelSize;

varying vec2 linePos;
varying float lineLength;

void main()
	{
	/* Calculate the fragment's distance from the line segment: */
	float lpx=max(abs(linePos.x)-lineLength*0.5,0.0);
	float lineDist=sqrt(linePos.y*linePos.y+lpx*lpx);
	
	/* Calculate the fragment's coverage: */
	float coverage=clamp(1.0-(lineDist-(lineWidth-pixelSize)*0.5)/pixelSize,0.0,1.0);
	
	/* Set the fragment's color: */
	gl_FragColor=vec4(gl_Color.rgb,gl_Color.a*coverage);
	}
