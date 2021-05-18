#version 420

uniform sampler2D   uParticleTexture;

in vec2				TexCoord0;
in vec3             vColor;
out vec4            outColor;

void main()
{
    vec3 particleColor = texture( uParticleTexture, TexCoord0 ).rgb;
    //outColor = vec4( vColor*particleColor, 1.0 );
    outColor = texture( uParticleTexture, gl_PointCoord );
    outColor.rgb *= vColor.rgb;
}