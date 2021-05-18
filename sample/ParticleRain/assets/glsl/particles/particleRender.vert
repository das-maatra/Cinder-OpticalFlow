#version 420
#extension GL_ARB_shader_storage_buffer_object : require

layout( location = 0 ) in int particleId;
in vec2			ciTexCoord0;
out vec3		vColor;
out vec2		TexCoord0;

struct Particle
{
	vec3	pos;
	vec3	ppos;
	vec3	home;
	vec4	color;
	float	damping;
	float	scale;
};

layout( std140, binding = 0 ) buffer Part
{
    Particle particles[];
};

uniform mat4 ciModelViewProjection;


void main()
{
	gl_Position = ciModelViewProjection * vec4( particles[particleId].pos, 1 );
	vColor = particles[particleId].color.rgb;
	TexCoord0 = ciTexCoord0;
	gl_PointSize = 1.0f * particles[particleId].scale;
}