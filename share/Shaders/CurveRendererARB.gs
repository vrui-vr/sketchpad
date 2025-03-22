/***********************************************************************
CurveTest.gs - Geometry shader for anti-aliased curve rendering.
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

#version 330

layout (lines) in;
layout (triangle_strip) out;
layout (max_vertices=8) out;

uniform float lineWidth;
uniform float pixelSize;

varying in vec2 vNormal[];
varying out vec2 linePos;
varying out vec2 v0,n0,v1,n1;
varying out vec2 modelPos;

void main()
	{
	/* Calculate the outer line half width: */
	float hw1=(lineWidth+pixelSize)*0.5;
	
	/* Calculate the line segment's direction and normal vectors: */
	v0=gl_PositionIn[0].xy;
	v1=gl_PositionIn[1].xy;
	vec2 v;
	if(v1==v0) // Use an arbitrary direction for zero-length line segments: */
		v=vec2(hw1,0.0);
	else
		v=normalize(v1-v0)*hw1;
	vec2 n=vec2(-v.y,v.x);
	n0=vNormal[0];
	n1=vNormal[1];
	
	/* Emit vertices for three quads representing the extended line segment: */
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(-hw1,hw1);
	modelPos=v0-v+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-v+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(-hw1,-hw1);
	modelPos=v0-v-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-v-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(0,hw1);
	modelPos=v0+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(0,-hw1);
	modelPos=v0-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(0,hw1);
	modelPos=v1+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(0,-hw1);
	modelPos=v1-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(hw1,hw1);
	modelPos=v1+v+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+v+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_FrontColorIn[0];
	linePos=vec2(hw1,-hw1);
	modelPos=v1+v-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+v-n,0.0,1.0);
	EmitVertex();
	}
