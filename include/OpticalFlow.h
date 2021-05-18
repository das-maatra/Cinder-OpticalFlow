/*

	OpticalFlow.h
		- Shuvashis Das | Red Paper Heart Studio

*/

#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

#include "Shaders.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OpticalFlow {
public:
	void							setup(int width, int height);
	void							update(gl::TextureRef cur);
	void							drawFlowGrid();
	void							drawBlur();
	void							drawReposition();
	void							drawLastTex();
	void							setLambda(float lambda) { mLambda = lambda;  }
	void							setBlur(float blurAmount) { mBlurAmount = blurAmount; }
	void							setDisplacement(float displaceAmount) { mDisplaceAmount = displaceAmount; }
	
	ci::gl::Texture2dRef			getOpticalFlowTexture() { return mFlowFbo->getColorTexture(); }			// raw OpticalFlow
	ci::gl::Texture2dRef			getOpticalFlowBlurTexture() { return mBlurVFbo->getColorTexture(); }	// blurred Optical Flow

	int								getWidth() { return mWidth; }
	int								getHeight(){ return mHeight;}

private:
	void							setupFbos();
	
	// width and hegiht for all FBOs/textures
	int								mWidth;
	int								mHeight;

	// variables specific to Optical Flow algorithm
	float							mLambda = 0.1;
	float							mBlurAmount = 5.0f;
	float							mDisplaceAmount = 0.05f;

	// shader program declaration
	Shaders							mShaders;
	
	// FBOs to draw texture on using the shaders
	gl::FboRef						mLastTex;
	gl::FboRef						mFlowFbo;
	gl::FboRef						mBlurHFbo;
	gl::FboRef						mBlurVFbo;
	gl::FboRef						mReposFbo;

};
