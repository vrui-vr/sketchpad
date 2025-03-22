/***********************************************************************
CurveTest.fs - Fragment shader for anti-aliased curve rendering.
Copyright (c) 2024 Oliver Kreylos
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
