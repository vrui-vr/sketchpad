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

layout (lines_adjacency) in;
layout (triangle_strip) out;
layout (max_vertices=4) out;

uniform float lineWidth;
uniform float pixelSize;

varying out vec2 linePos;
varying out float lineLength;

void main()
	{
	/* Calculate the outer line half width: */
	float hw1=(lineWidth+pixelSize)*0.5;
	
	/* Calculate the three segment direction vectors: */
	vec2 d0=gl_PositionIn[1].xy-gl_PositionIn[0].xy;
	vec2 d1=gl_PositionIn[2].xy-gl_PositionIn[1].xy;
	vec2 d2=gl_PositionIn[3].xy-gl_PositionIn[2].xy;
	
	/* Calculate the quad's generating directions: */
	float d1Len=length(d1);
	vec2 x;
	if(d1Len!=0.0)
		x=d1*(hlw/d1Len);
	else
		x=vec2(hlw,0.0);
	vec2 y=vec2(-x.y,x.x);
	
	/* Generate the quad's two starting vertices: */
	float d0d1=dot(d0,d1);
	if(d0d1>0.0)
		{
		/* Calculate the half-angle vector between the line segment and its left neighbor: */
		float d0Len=length(d0);
		vec2 h=d0/d0Len+d1/d1Len;
		float alpha=acos(d0d1/(d0Len*d1Len));
		h=h*(hlw/(length(h)*cos(alpha*0.5)));
		vec2 half=vec2(-h.y,h.x);
		
		float xoff=hlw*tan(alpha*0.5);
		if(d0.x*d1.y-d0.y*d1.x<0.0)
			xoff=-xoff;
		
		/* Emit the top left vertex: */
		gl_FrontColor=gl_FrontColorIn[1];
		linePos=vec2(-d1Len*0.5+xoff,hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[1]+vec4(half,0.0,0.0));
		EmitVertex();
		
		/* Emit the bottom left vertex: */
		gl_FrontColor=gl_FrontColorIn[1];
		linePos=vec2(-d1Len*0.5-xoff,-hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[1]+vec4(-half,0.0,0.0));
		EmitVertex();
		}
	else
		{
		/* Add a full end cap to the left of the quad: */
		
		/* Emit the top left vertex: */
		gl_FrontColor=gl_FrontColorIn[1];
		linePos=vec2(-d1Len*0.5-hlw,hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[1]+vec4(-x+y,0.0,0.0));
		EmitVertex();
		
		/* Emit the bottom left vertex: */
		gl_FrontColor=gl_FrontColorIn[1];
		linePos=vec2(-d1Len*0.5-hlw,-hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[1]+vec4(-x-y,0.0,0.0));
		EmitVertex();
		}
	
	/* Generate the quad's two ending vertices: */
	float d1d2=dot(d1,d2);
	if(d1d2>0.0)
		{
		/* Calculate the half-angle vector between the line segment and its left neighbor: */
		float d2Len=length(d2);
		vec2 h=d1/d1Len+d2/d2Len;
		float alpha=acos(d1d2/(d1Len*d2Len));
		h=h*(hlw/(length(h)*cos(alpha*0.5)));
		vec2 half=vec2(-h.y,h.x);
		
		float xoff=hlw*tan(alpha*0.5);
		if(d1.x*d2.y-d1.y*d2.x<0.0)
			xoff=-xoff;
		
		/* Emit the top left vertex: */
		gl_FrontColor=gl_FrontColorIn[2];
		linePos=vec2(d1Len*0.5-xoff,hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[2]+vec4(half,0.0,0.0));
		EmitVertex();
		
		/* Emit the bottom left vertex: */
		gl_FrontColor=gl_FrontColorIn[2];
		linePos=vec2(d1Len*0.5+xoff,-hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[2]+vec4(-half,0.0,0.0));
		EmitVertex();
		}
	else
		{
		/* Add a full end cap to the right of the quad: */
		
		/* Emit the top right vertex: */
		gl_FrontColor=gl_FrontColorIn[2];
		linePos=vec2(d1Len*0.5+hlw,hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[2]+vec4(x+y,0.0,0.0));
		EmitVertex();
		
		/* Emit the bottom left vertex: */
		gl_FrontColor=gl_FrontColorIn[2];
		linePos=vec2(d1Len*0.5+hlw,-hlw);
		lineLength=d1Len;
		gl_Position=gl_ModelViewProjectionMatrix*(gl_PositionIn[2]+vec4(x-y,0.0,0.0));
		EmitVertex();
		}
	
	EndPrimitive();
	}
