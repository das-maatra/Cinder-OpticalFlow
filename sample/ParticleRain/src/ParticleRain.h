#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/Log.h"
#include "cinder/CinderMath.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#pragma pack( push, 1 )
struct Particle
{
	vec3	pos;
	float   pad1;
	vec3	ppos;
	float   pad2;
	vec3	home;
	float   pad3;
	vec4    color;
	float	damping;
	float	scale;
	vec2	pad4;
};
#pragma pack( pop )

const int NUM_PARTICLES = static_cast<int>(1e4);


class ParticleRain{
	
	public:
		ParticleRain() {};
		~ParticleRain() {};

		void setup( int resX, int resY );
		void update(ci::gl::Texture2dRef opticalFlowTex);
		void draw();

		// Mouse state suitable for passing as uniforms to update program
		bool					mMouseDown = false;
		float					mMouseForce = 0.0f;
		float					mWindowHeight = float(getWindowHeight());
		vec3					mMousePos = vec3(0);

	private:
		float					mGravity = 03.0;
		int						mResX;
		int						mResY;
		float					mForceMult = 100.0f;

		enum { WORK_GROUP_SIZE = 8, };
		gl::GlslProgRef			mRenderProg;
		gl::GlslProgRef			mUpdateProg;

		gl::TextureRef			mParticleTexture;

		// Buffers holding raw particle data on GPU.
		gl::SsboRef				mParticleBuffer;
		gl::VboRef				mIdsVbo;
		gl::VaoRef				mAttributes;

		
};

