/*

	OpticalFlow.cpp
		- Shuvashis Das | Red Paper Heart Studio

*/


#include "OpticalFlow.h"

void OpticalFlow::setup(int width, int height) {

	//setting up variables with arguments or default values
	mWidth = width;
	mHeight = height;
	setupFbos();
	mShaders.setup();
}

void OpticalFlow::setupFbos() {

	gl::Fbo::Format format;
	format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	format.setColorTextureFormat(gl::Texture::Format().internalFormat(GL_RGBA));
	mLastTex = gl::Fbo::create(mWidth, mHeight, format);
	mFlowFbo = gl::Fbo::create(mWidth, mHeight, format);
	mBlurHFbo = gl::Fbo::create(mWidth, mHeight, format);
	mBlurVFbo = gl::Fbo::create(mWidth, mHeight, format);
	mReposFbo = gl::Fbo::create(mWidth, mHeight, format);
}

void OpticalFlow::update(gl::TextureRef cur) {
	
	//flow Process
	{
		gl::clear(Color::black());
		gl::ScopedFramebuffer fbScpFlow(mFlowFbo);
		gl::ScopedViewport scpVpFlow(ivec2(0), mFlowFbo->getSize());
		gl::ScopedMatrices matFlow;
		//gl::ScopedBlendAlpha alphaBlend;
		gl::setMatricesWindow(mFlowFbo->getSize());

		mShaders.mFlowShader->uniform("uScale", vec2(1, 1));
		mShaders.mFlowShader->uniform("uOffset", vec2(0.01));
		mShaders.mFlowShader->uniform("uLambda", mLambda);
		mShaders.mFlowShader->uniform("tex0", 0);
		mShaders.mFlowShader->uniform("tex1", 1);
		gl::ScopedTextureBind tex0(cur, uint8_t(0));
		gl::ScopedTextureBind tex1(mLastTex->getColorTexture(), uint8_t(1));
		gl::ScopedGlslProg shaderFlow(mShaders.mFlowShader);
		
		gl::drawSolidRect(mFlowFbo->getBounds());
	}
	//blur Process
	{
		float sigma = mBlurAmount / 2.0;
		float horizontalPass = 0.0
			;
		gl::clear(Color::black());
		gl::ScopedFramebuffer fbScpBlurH(mBlurHFbo);
		gl::ScopedViewport scpVpBlurH(ivec2(0), mBlurHFbo->getSize());
		gl::ScopedMatrices matBlurH;
		//gl::ScopedBlendAlpha alphaBlendBlurH;
		gl::setMatricesWindow(mBlurHFbo->getSize());
		
		gl::ScopedGlslProg shaderBlurH(mShaders.mBlurShader);
		mShaders.mFlowShader->uniform("uInTexture", 0);
		gl::ScopedTextureBind texScopeBlurH(mFlowFbo->getColorTexture(), (uint8_t)0);
		mShaders.mBlurShader->uniform("uBlurSize", mBlurAmount);
		mShaders.mBlurShader->uniform("uSigma", sigma);
		mShaders.mBlurShader->uniform("uTexOffset", vec2(2.0));
		mShaders.mBlurShader->uniform("uHorizontalPass", horizontalPass);
		gl::drawSolidRect(mBlurHFbo->getBounds());

		horizontalPass = 0.0; //vertical is 0
		gl::ScopedFramebuffer fbScpBlurV(mBlurVFbo);
		gl::ScopedViewport scpVpBlurV(ivec2(0), mBlurVFbo->getSize());
		gl::ScopedMatrices matBlurV;
		//gl::ScopedBlendAlpha alphaBlendBlurV;

		gl::clear(Color::black());
		gl::ScopedGlslProg shaderBlurV(mShaders.mBlurShader);
		mShaders.mFlowShader->uniform("uInTexture", 0);
		gl::ScopedTextureBind texScopeBlurV(mBlurHFbo->getColorTexture(), 0);
		mShaders.mBlurShader->uniform("uBlurSize", mBlurAmount);
		mShaders.mBlurShader->uniform("uSigma", sigma);
		mShaders.mBlurShader->uniform("uTexOffset", vec2(2.0));
		mShaders.mBlurShader->uniform("uHorizontalPass", horizontalPass);
		gl::setMatricesWindow(mBlurVFbo->getSize());
		gl::drawSolidRect(mBlurVFbo->getBounds());
	}

	//repos Process
	{
		gl::ScopedFramebuffer fbScpRepos(mReposFbo);
		gl::ScopedViewport scpVpRepos(ivec2(0), mReposFbo->getSize());
		gl::ScopedMatrices matRepos;
		//gl::ScopedBlendAlpha alphaBlend;
		gl::clear(Color::black());

		gl::ScopedGlslProg shaderRepos(mShaders.mReposShader);
		mShaders.mFlowShader->uniform("tex0", 0);
		mShaders.mFlowShader->uniform("tex1", 1);
		gl::ScopedTextureBind texScopeRepos(cur, 0);
		gl::ScopedTextureBind texScopeRepos1(mBlurVFbo->getColorTexture(), (uint8_t)1);
		mShaders.mReposShader->uniform("uAmount", vec2(mDisplaceAmount));
		gl::setMatricesWindow(mReposFbo->getSize());
		gl::drawSolidRect(mReposFbo->getBounds());

	}

	//last frame
	{
		gl::clear(Color::black());
		gl::ScopedFramebuffer fbScpLastTex(mLastTex);
		gl::ScopedViewport scpVpLastTex(ivec2(0), mLastTex->getSize());
		gl::ScopedMatrices matLastTex;
		gl::setMatricesWindow(mLastTex->getSize());
		gl::draw(cur);
	}

}

void OpticalFlow::drawFlowGrid() {
	gl::draw(mFlowFbo->getColorTexture());
}

void OpticalFlow::drawLastTex() {
	gl::draw(mLastTex->getColorTexture());
}

void OpticalFlow::drawBlur() {
	gl::draw(mBlurVFbo->getColorTexture());
}

void OpticalFlow::drawReposition() {
	gl::draw(mReposFbo->getColorTexture());
}

