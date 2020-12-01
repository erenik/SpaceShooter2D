// Author: Emil Hedemalm
// Date: 2012-10-29
// Name: Simple UI Shader
#version 120

// Uniforms
// 2D Texture texture
uniform sampler2D baseImage;

// Yush.
uniform vec4	primaryColorVec4 = vec4(1,1,1,1);
/// Highlight that is added linearly upon the final product.
uniform vec4	highlightColorVec4 = vec4(0,0,0,0);

// Input data from the fragment shader
varying vec2 UV_Coord;	// Just passed on
varying vec3 worldCoord;	// World coordinates of the fragment
varying vec3 vecToEye;	// Vector from vertex to eye
varying vec3 position;

void main(void) 
{
	// Texture image data. This will be the base for the colors.
	vec4 baseFrag = texture2D(baseImage, UV_Coord);
	vec4 color = clamp(baseFrag, vec4(0,0,0,0), vec4(1,1,1,1));
	
//	if (highlightColorVec4.x > 0)
	//	color.x += 0.5;
	
	
	gl_FragColor = color;
	gl_FragColor *= primaryColorVec4;
	float highlightFactor = 1.0 + highlightColorVec4.x;
	gl_FragColor.xyz *= highlightFactor;
}


