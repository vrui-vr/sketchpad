/***********************************************************************
CurveTest.vs - Vertex shader for anti-aliased curve rendering.
Copyright (c) 2024 Oliver Kreylos
***********************************************************************/

varying vec2 vNormal;

void main()
	{
	/* Pass vertex color, normal, and model-space position to geometry shader: */
	gl_FrontColor=gl_Color;
	vNormal=gl_Normal.xy;
	gl_Position=gl_Vertex;
	}
