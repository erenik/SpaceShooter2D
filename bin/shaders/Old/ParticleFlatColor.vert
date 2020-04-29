// Author: Emil Hedemalm
// Date: 2014-09-18
// A shader meant for billboard-type particles, where a model (a plane) is instanced to multiple locations and rendered at once via instanced rendering.
#version 120

// Uniforms
// Model, view and projection matrices provided by the client.
// uniform mat4 modelMatrix = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
//uniform mat4 viewMatrix = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
//uniform mat4 projectionMatrix = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
uniform mat4 viewProjectionMatrix = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);

uniform vec3 cameraRightWorldSpace = vec3(1,0,0);
uniform vec3 cameraUpWorldSpace = vec3(0,1,0);

// Input data for the shader
// in_Position was bound to attribute index 0, UV to index 1 and Normals to index 2.
attribute vec4 in_VertexPosition; // Position of vertex.
attribute vec4 in_ParticlePositionScale; // Position of the particle and it's scale in the w-component.
attribute vec4 in_Color;
attribute vec4 in_ParticleLifeTimeDurationScale; //Life time (total) and duration (current)

// Output data for the fragment shader
varying vec2 UV_Coord;		// Just passed on
varying vec4 position;
varying vec4 particleColorMultiplier;
varying vec2 lifeTimeDuration;

// https://www.opengl.org/sdk/docs/man/html/glDrawArraysInstanced.xhtml
// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/

void main()
{
	vec2 billboardSize = in_ParticleLifeTimeDurationScale.zw;
	lifeTimeDuration = in_ParticleLifeTimeDurationScale.xy;	
			
	UV_Coord = in_VertexPosition.xy + vec2(0.5, 0.5);
	
	vec4 particleCenter_worldSpace = vec4(in_ParticlePositionScale.xyz, 1);
	vec4 vertexPosition_worldSpace = vec4(particleCenter_worldSpace.xyz, 1);
	// Add in camera-X
//	cameraRightWorldSpace = vec4(1,0,0,1);
//	vec4 cameraRight = vec4(1,0,0,1);
	vec3 cameraRight = cameraRightWorldSpace;
	
	vertexPosition_worldSpace.xyz += cameraRight * in_VertexPosition.x * billboardSize.x;

	// Add in camera-Y
	// cameraUpWorldSpace
//	vec4 cameraUp = vec4(0,1,0,0);
	vec3 cameraUp = cameraUpWorldSpace;
	vertexPosition_worldSpace.xyz += cameraUp * in_VertexPosition.y * billboardSize.y;
		
	vertexPosition_worldSpace.w = 1;
	
//	mat4 viewProjectionMatrix2 =  projectionMatrix * viewMatrix;
	position = viewProjectionMatrix * vertexPosition_worldSpace;	
    gl_Position = position;
	particleColorMultiplier = in_Color;	
}

