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
varying vec2 v0,n0,v1,n1;
varying vec2 modelPos;

void main()
	{
	/* Check the fragment against the vertex planes: */
	if((n0!=vec2(0.0,0.0)&&dot(modelPos-v0,n0)<0.0)||(n1!=vec2(0.0,0.0)&&dot(modelPos-v1,n1)>=0.0))
		discard;
	
	/* Calculate the fragment's coverage: */
	float coverage=clamp(1.0-(length(linePos)-(lineWidth-pixelSize)*0.5)/pixelSize,0.0,1.0);
	
	/* Set the fragment's color: */
	gl_FragColor=vec4(gl_Color.rgb,gl_Color.a*coverage);
	}
