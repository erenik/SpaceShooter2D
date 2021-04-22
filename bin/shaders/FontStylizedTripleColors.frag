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

	if (primaryColorVec4.x > 0.9 && primaryColorVec4.y > 0.9) {
		// Selected: c7dcd0, 9babb2, 7f708a
		if (baseFrag.x > 0.8)
			gl_FragColor = vec4(0.7804, 0.8627, 0.8157, 1);
		else if (baseFrag.x > 0.5)
			gl_FragColor = vec4(0.6078, 0.6706, 0.6980, 1);
		else
			gl_FragColor = vec4(0.4980, 0.4392, 0.5412, 1);
	}
	else if (primaryColorVec4.x > 0.90){
		// Orange 
		// Highlight: #f9c22b
		// Color: #fb6b1d
		// Shadow: #ae2334
		if (baseFrag.x > 0.8)
			gl_FragColor = vec4(0.9765, 0.7608, 0.1686,1);
		else if (baseFrag.x > 0.5)
			gl_FragColor = vec4(0.9843, 0.4196, 0.1137,1);
		else
			gl_FragColor = vec4(0.6824, 0.1373, 0.2039,1);
	}
	else if (primaryColorVec4.x < 0.6 && primaryColorVec4.z < 0.8){
		// Purple, f45d92, 8f1767, 5e1c5a
		if (baseFrag.x > 0.8)
			gl_FragColor = vec4(0.9569, 0.3647, 0.5725, 1);
		else if (baseFrag.x > 0.5)
			gl_FragColor = vec4(0.5608, 0.0902, 0.4039, 1);
		else
			gl_FragColor = vec4(0.3686, 0.1098, 0.3529, 1);
	}
	else if (primaryColorVec4.x > 0.3) {
		// Deselected 7f708a, 625565, 3e3546
		if (baseFrag.x > 0.8)
			gl_FragColor = vec4(0.4980, 0.4392, 0.5412, 1);
		else if (baseFrag.x > 0.5)
			gl_FragColor = vec4(0.3843, 0.3333, 0.3961, 1);
		else
			gl_FragColor = vec4(0.2431, 0.2078, 0.2745, 1);
	}
	else {
		gl_FragColor *= primaryColorVec4 * 0.5 + 0.5;
		gl_FragColor += highlightColorVec4;	
	}
	// fb6b1dff

	//gl_FragColor.x += 1.0;
	//gl_FragColor.w = 1;
	
	if (debugColor.x > 0)
		gl_FragColor = vec4(debugColor.xyz, 1);

	gl_FragColor.w = primaryColorVec4.w;
}


