/***********************************************************************
CurveTest.gs - Geometry shader for anti-aliased curve rendering using
OpenGL 3.2 core shaders.
Copyright (c) 2024-2025 Oliver Kreylos
***********************************************************************/

#version 150 compatibility

layout (lines) in;
layout (triangle_strip,max_vertices=8) out;

uniform float lineWidth;
uniform float pixelSize;

in vec2 vNormal[];
out vec2 linePos;
out vec2 v0,n0,v1,n1;
out vec2 modelPos;

void main()
	{
	/* Calculate the outer line half width: */
	float hw1=(lineWidth+pixelSize)*0.5;
	
	/* Calculate the line segment's direction and normal vectors: */
	v0=gl_in[0].gl_Position.xy;
	v1=gl_in[1].gl_Position.xy;
	vec2 v;
	if(v1==v0) // Use an arbitrary direction for zero-length line segments: */
		v=vec2(hw1,0.0);
	else
		v=normalize(v1-v0)*hw1;
	vec2 n=vec2(-v.y,v.x);
	n0=vNormal[0];
	n1=vNormal[1];
	
	/* Emit vertices for three quads representing the extended line segment: */
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(-hw1,hw1);
	modelPos=v0-v+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-v+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(-hw1,-hw1);
	modelPos=v0-v-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-v-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(0,hw1);
	modelPos=v0+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(0,-hw1);
	modelPos=v0-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v0-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(0,hw1);
	modelPos=v1+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(0,-hw1);
	modelPos=v1-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1-n,0.0,1.0);
	EmitVertex();
	
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(hw1,hw1);
	modelPos=v1+v+n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+v+n,0.0,1.0);
	EmitVertex();
	gl_FrontColor=gl_in[0].gl_FrontColor;
	linePos=vec2(hw1,-hw1);
	modelPos=v1+v-n;
	gl_Position=gl_ModelViewProjectionMatrix*vec4(v1+v-n,0.0,1.0);
	EmitVertex();
	
	EndPrimitive();
	}
