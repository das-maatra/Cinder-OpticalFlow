/*

	ParticleRender.geom
		- Shuvashis Das 02/13/2020

*/

#version 150

layout(points) in;
layout(line_strip, max_vertices = 64) out;

uniform float uParticleSize;
uniform mat4 ciProjectionMatrix;

in vData{
	vec4	initPos;
}vertices[];

void drawLines(){
	for(int i = 0;i < gl_in.length();i++){
		gl_Position = ciProjectionMatrix * (gl_in[i].gl_Position );
		EmitVertex();
		gl_Position = ciProjectionMatrix * vertices[i].initPos ;
		EmitVertex();
	}
	EndPrimitive();
}

void drawRectangle(){
	for (int i = 0; i < gl_in.length (); i++) {
		gl_Position = ciProjectionMatrix* (gl_in [i].gl_Position + vec4 (-uParticleSize,  uParticleSize, 0.0, 0.0) );
		EmitVertex   ();
		gl_Position = ciProjectionMatrix* (gl_in [i].gl_Position + vec4 ( uParticleSize,  uParticleSize, 0.0, 0.0) );
		EmitVertex   ();
		gl_Position = ciProjectionMatrix* (gl_in [i].gl_Position + vec4 ( uParticleSize, -uParticleSize, 0.0, 0.0) );
		EmitVertex   ();
		gl_Position = ciProjectionMatrix* (gl_in [i].gl_Position + vec4 (-uParticleSize, -uParticleSize, 0.0, 0.0) );
		EmitVertex   ();
		gl_Position = ciProjectionMatrix* (gl_in [i].gl_Position + vec4 (-uParticleSize,  uParticleSize, 0.0, 0.0) );
		EmitVertex   ();
	  }
	  EndPrimitive ();
}

void main(){
	// drawing rectangle at particle's current position 
	drawRectangle();

	// drawing line from particle's initial position to current particle position
	drawLines();	
}