#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

class Shaders {
public:
	void					setup();
	
	ci::gl::GlslProgRef		mFlowShader;
	ci::gl::GlslProgRef		mBlurShader;
	ci::gl::GlslProgRef		mReposShader;

};