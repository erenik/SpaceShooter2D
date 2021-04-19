// Author: Emil Hedemalm
// Date: 2012-10-29
// Name: Simple UI Shader
#version 120

// Uniforms
// 2D Texture texture
uniform sampler2D baseImage;

// Yush.
uniform vec4 primaryColorVec4 = vec4(1,1,1,1);
uniform vec4 secondaryColorVec4 = vec4(.5,.5,.5,1);
/// Highlight that is added linearly upon the final product.
uniform vec4 highlightColorVec4 = vec4(0,0,0,0);

// Input data from the fragment shader
varying vec2 UV_Coord;	// Just passed on
varying vec3 worldCoord;	// World coordinates of the fragment
varying vec3 vecToEye;	// Vector from vertex to eye
varying vec3 position;

varying vec3 debugColor;

/// 0 for No, 1 for Yes
uniform int hoveredOver;

/** Default 0 - pass through.
	1 - Simple White font - apply primaryColorVec4 multiplicatively
	2 - Replacer. Replaces a set amount of colors in the font for other designated colors (primarily primaryColorVec4 and se)
*/
uniform int colorEquation = 1;

void main(void) 
{
	// Texture image data. This will be the base for the colors.
	vec4 baseFrag = texture2D(baseImage, UV_Coord);
	
	gl_FragColor = baseFrag;

	if (baseFrag.w == 0)
		return;

	gl_FragColor.xyz = primaryColorVec4.xyz;

	gl_FragColor += highlightColorVec4;	
	// fb6b1dff

	//gl_FragColor.x = 0.5;
	
	if (debugColor.x > 0)
		gl_FragColor = vec4(debugColor.xyz, 1);

	gl_FragColor.w = primaryColorVec4.w;
}


